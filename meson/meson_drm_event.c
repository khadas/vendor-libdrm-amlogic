/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <libudev.h>
#include <linux/string.h>

#include "meson_drm_event.h"
#include "meson_drm_display.h"

#ifndef XDG_RUNTIME_DIR
#define XDG_RUNTIME_DIR     "/run"
#endif
#define LIBUDEV_EVT_TYPE_KERNEL     "kernel"
#define LIBUDEV_SUBSYSTEM_DRM       "drm"

//static pthread_t event_monitor_threadId;
static bool isMonitoringAlive = false;
displayEventCallback _DisplayEventCb = NULL;
static void* uevent_monitor_thread(void *arg);

void startDisplayUeventMonitor()
{
    int err = -1;
    isMonitoringAlive = true;
    pthread_t event_monitor_threadId;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    err = pthread_create (&event_monitor_threadId, &attr,uevent_monitor_thread,NULL);
    if (err) {
        printf("DSHAL : Failed to Ceate HDMI Hot Plug Thread ....\r\n");
        event_monitor_threadId = -1;
    }
}
void stopDisplayUeventMonitor()
{
    isMonitoringAlive = false;
}

bool RegisterDisplayEventCallback(displayEventCallback cb)
{
    bool ret = true;
    if (cb == NULL) {
        printf("ERROR[%s:%d] argument NULL\n", __FUNCTION__, __LINE__);
        ret = false;
    } else {
        _DisplayEventCb = cb;
    }
    return ret;
}
bool get_hdcp_status(ENUM_HDCP_STATUS *status)
{
    // need to do:get HDCP status from property:
    // Content Protection,HDCP Content Type, HDCP_CONTENT_TYPE0_PRIORITY
	*status = HDCP_STATUS_AUTHENTICATED;
	return true;
}

static void* uevent_monitor_thread(void *arg)
{
    printf("[%s:%d]start\n", __FUNCTION__, __LINE__);
    bool wasConnected = false;
    struct udev *udev = NULL;
    struct udev_device *dev = NULL;
    struct udev_monitor *mon = NULL;
    ENUM_MESON_DRM_CONNECTION enConnection = MESON_DRM_UNKNOWNCONNECTION;
    ENUM_MESON_DRM_CONNECTION enPreConnection = MESON_DRM_UNKNOWNCONNECTION;
    ENUM_DISPLAY_EVENT enDisplayEvent = DISPLAY_EVENT_MAX;
    ENUM_HDCP_STATUS enPreStatus = HDCP_STATUS_MAX;
    ENUM_HDCP_STATUS enCurStatus = HDCP_STATUS_MAX;
    /* create udev object */
    udev = udev_new();
    if (!udev) {
        printf("ERROR[%s:%d] Can't create udev monitor for DRM,\n", __FUNCTION__, __LINE__);
    } else {
        mon = udev_monitor_new_from_netlink(udev, LIBUDEV_EVT_TYPE_KERNEL);
        if (mon) {
            int fd = -1;
            fd_set fds;
            struct timeval tv;
            int ret;
            if ((fd = udev_monitor_get_fd(mon)) < 0) {
                printf("ERROR[%s:%d] udev_monitor_get_fd failed,\n", __FUNCTION__, __LINE__);
            } else {
                if (udev_monitor_filter_add_match_subsystem_devtype(mon, LIBUDEV_SUBSYSTEM_DRM, NULL) < 0) {
                    printf("ERROR[%s:%d] udev_monitor_filter_add_match_subsystem_devtype failed,\n", __FUNCTION__, __LINE__);
                } else {
                    if (udev_monitor_enable_receiving(mon) < 0) {
                        printf("ERROR[%s:%d] udev_monitor_enable_receiving\n", __FUNCTION__, __LINE__);
                    } else {
                        while (isMonitoringAlive) {
                            FD_ZERO(&fds);
                            FD_SET(fd, &fds);
                            tv.tv_sec = 5; /* FIXME: fine tune the select timeout. */
                            tv.tv_usec = 0;
                            ret = select(fd+1, &fds, NULL, NULL, &tv);
                            if (ret > 0 && FD_ISSET(fd, &fds)) {
                                dev = udev_monitor_receive_device(mon);
                                if (dev) {
                                    if (!strcmp(udev_device_get_action(dev), "change")) {
                                        printf("I: ACTION=%s\n", udev_device_get_action(dev));
                                        printf("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
                                        printf("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
                                        enConnection = meson_drm_getConnection();
                                        if ( enPreConnection != enConnection) {
                                            enPreConnection = enConnection;
                                            printf("Send %s HDMI Hot Plug Event !!!\n",
                                                    (enConnection ? "Connect":"DisConnect"));
                                            enDisplayEvent = enConnection ? DISPLAY_EVENT_CONNECTED:DISPLAY_EVENT_DISCONNECTED;
                                            if (_DisplayEventCb) {
                                                _DisplayEventCb( enDisplayEvent, NULL);
                                            }
                                        }
                                        if ( !get_hdcp_status(&enCurStatus) )
                                            printf("%s:%d: get_hdcp_status fail\n", __func__, __LINE__);
                                        if ( enCurStatus != enPreStatus ) {
                                            enPreStatus = enCurStatus;
                                            enDisplayEvent = enCurStatus ? DISPLAY_HDCP_AUTHENTICATIONFAILURE:DISPLAY_HDCP_AUTHENTICATED;
                                            printf("Send %s !!!\n", (enCurStatus ? "DISPLAY_HDCP_AUTHENTICATIONFAILURE":"DISPLAY_HDCP_AUTHENTICATED"));
                                            if (_DisplayEventCb) {
                                                _DisplayEventCb( enDisplayEvent, NULL);
                                            }
                                        }
                                    }
                                    /* free dev */
                                    udev_device_unref(dev);
                                }
                            } else {
                                /* TODO: Select timeout or error; handle accordingly. */
                            }
                        }
                    }
                }
            }
        } else {
            printf("ERROR[%s:%d] udev_monitor_new_from_netlink failed\n", __FUNCTION__, __LINE__);
        }
    }
    udev = NULL;
    dev = NULL;
    mon = NULL;
    pthread_exit(NULL);
    return NULL;
}

