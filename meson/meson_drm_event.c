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
#include "meson_drm_settings.h"
#include "meson_drm_log.h"

#ifndef XDG_RUNTIME_DIR
#define XDG_RUNTIME_DIR     "/run"
#endif
#define LIBUDEV_EVT_TYPE_KERNEL     "kernel"
#define LIBUDEV_SUBSYSTEM_DRM       "drm"

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
    DEBUG("[%s:%d]", __FUNCTION__, __LINE__);
    err = pthread_create (&event_monitor_threadId, &attr,uevent_monitor_thread,NULL);
    if (err) {
        ERROR("%s %d DSHAL : Failed to Ceate HDMI Hot Plug Thread ....\r",__FUNCTION__, __LINE__);
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
        ERROR("ERROR[%s:%d] argument NULL",__FUNCTION__, __LINE__);
        ret = false;
    } else {
        _DisplayEventCb = cb;
    }
    return ret;
}
bool get_hdcp_status()
{
    bool ret = false;
    int fd = meson_open_drm();
    if (MESON_AUTH_STATUS_SUCCESS == meson_drm_getHdcpAuthStatus(fd, MESON_CONNECTOR_HDMIA)) {
        ret = true;
    }
    meson_close_drm(fd);
    return ret;
}

static void* uevent_monitor_thread(void *arg)
{
    DEBUG("[%s:%d]start", __FUNCTION__, __LINE__);
    bool wasConnected = false;
    struct udev *udev = NULL;
    struct udev_device *dev = NULL;
    struct udev_monitor *mon = NULL;
    ENUM_MESON_CONN_CONNECTION enConnection = MESON_UNKNOWNCONNECTION;
    ENUM_MESON_CONN_CONNECTION enPreConnection = MESON_UNKNOWNCONNECTION;
    ENUM_DISPLAY_EVENT enDisplayEvent = DISPLAY_EVENT_MAX;
    bool enPreStatus = false;
    bool enCurStatus = false;
    int drm_fd = meson_open_drm();
    /* create udev object */
    udev = udev_new();
    if (!udev) {
        ERROR("ERROR[%s:%d] Can't create udev monitor for DRM", __FUNCTION__, __LINE__);
    } else {
        mon = udev_monitor_new_from_netlink(udev, LIBUDEV_EVT_TYPE_KERNEL);
        if (mon) {
            int fd = -1;
            fd_set fds;
            struct timeval tv;
            int ret;
            bool isHPD;
            bool isConnectorEvent;
            if ((fd = udev_monitor_get_fd(mon)) < 0) {
                ERROR("ERROR[%s:%d] udev_monitor_get_fd failed", __FUNCTION__, __LINE__);
            } else {
                if (udev_monitor_filter_add_match_subsystem_devtype(mon, LIBUDEV_SUBSYSTEM_DRM, NULL) < 0) {
                    ERROR("ERROR[%s:%d] udev_monitor_filter_add_match_subsystem_devtype failed", __FUNCTION__, __LINE__);
                } else {
                    if (udev_monitor_enable_receiving(mon) < 0) {
                        ERROR("ERROR[%s:%d] udev_monitor_enable_receiving", __FUNCTION__, __LINE__);
                    } else {
                        while (isMonitoringAlive) {
                            isHPD = false;
                            isConnectorEvent = false;
                            FD_ZERO(&fds);
                            FD_SET(fd, &fds);
                            tv.tv_sec = 5; /* FIXME: fine tune the select timeout. */
                            tv.tv_usec = 0;
                            ret = select(fd+1, &fds, NULL, NULL, &tv);
                            if (ret > 0 && FD_ISSET(fd, &fds)) {
                                dev = udev_monitor_receive_device(mon);
                                if (dev) {
                                    if (!strcmp(udev_device_get_action(dev), "change")) {
                                        DEBUG("%s %d I: ACTION=%s",__FUNCTION__, __LINE__, udev_device_get_action(dev));
                                        DEBUG("%s %d I: DEVNAME=%s",__FUNCTION__, __LINE__, udev_device_get_sysname(dev));
                                        DEBUG("%s %d I: DEVPATH=%s",__FUNCTION__, __LINE__, udev_device_get_devpath(dev));
                                        struct udev_list_entry *list_entry;
                                        udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(dev)) {
                                            INFO("%s=%s\n",
                                                   udev_list_entry_get_name(list_entry),
                                                   udev_list_entry_get_value(list_entry));
                                            if (!strcmp(udev_list_entry_get_name(list_entry), "HOTPLUG"))
                                                isHPD = true;
                                            if (!strcmp(udev_list_entry_get_name(list_entry), "CONNECTOR"))
                                                isConnectorEvent = true;
                                        }
                                        if (isHPD && !isConnectorEvent) {
                                            enConnection = meson_drm_getConnectionStatus(drm_fd, MESON_CONNECTOR_HDMIA);
                                            if ( enPreConnection != enConnection) {
                                                enPreConnection = enConnection;
                                                INFO("%s %d Send %s HDMI Hot Plug Event !!!",__FUNCTION__, __LINE__,
                                                        (enConnection ? "Connect":"DisConnect"));
                                                enDisplayEvent = enConnection ? DISPLAY_EVENT_CONNECTED:DISPLAY_EVENT_DISCONNECTED;
                                                if (_DisplayEventCb) {
                                                    _DisplayEventCb( enDisplayEvent, NULL);
                                                }
                                            }
                                        }
                                        enCurStatus = get_hdcp_status();
                                        if ( enCurStatus != enPreStatus ) {
                                            enPreStatus = enCurStatus;
                                            enDisplayEvent = enCurStatus ? DISPLAY_HDCP_AUTHENTICATED:DISPLAY_HDCP_AUTHENTICATIONFAILURE;
                                            INFO("%s %d Send %s !!!\n",__FUNCTION__, __LINE__, (enCurStatus ? "DISPLAY_HDCP_AUTHENTICATED":"DISPLAY_HDCP_AUTHENTICATIONFAILURE"));
                                            if (_DisplayEventCb) {
                                                _DisplayEventCb( enDisplayEvent, NULL);
                                            }
                                        }
                                    }
                                    /* free dev */
                                    udev_device_unref(dev);
                                }
                                else {
                                    INFO("I:[%s:%d] no udev_monitor_receive_device ", __FUNCTION__, __LINE__);
                                    enConnection = MESON_UNKNOWNCONNECTION;
                                    enPreConnection = MESON_UNKNOWNCONNECTION;
                                }
                            } else {
                                /* TODO: Select timeout or error; handle accordingly. */
                            }
                        }
                    }
                }
            }
        } else {
            ERROR("ERROR[%s:%d] udev_monitor_new_from_netlink failed", __FUNCTION__, __LINE__);
        }
    }
    udev = NULL;
    dev = NULL;
    mon = NULL;
    meson_close_drm(drm_fd);
    pthread_exit(NULL);
    return NULL;
}
