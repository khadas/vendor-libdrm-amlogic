 /*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef MESON_DRM_EVENT_H_
#define MESON_DRM_EVENT_H_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#if defined(__cplusplus)
extern "C" {
#endif

typedef enum _ENUM_DISPLAY_EVENT {
    DISPLAY_EVENT_CONNECTED = 0,         //!< Display connected event.
    DISPLAY_EVENT_DISCONNECTED,          //!< Display disconnected event.
    DISPLAY_HDCP_AUTHENTICATED,          //!< HDCP authenticate success
    DISPLAY_HDCP_AUTHENTICATIONFAILURE,      //!< Rx Sense OFF event
    DISPLAY_EVENT_MAX,                   //!<MAX
}ENUM_DISPLAY_EVENT;

typedef enum _ENUM_HDCP_STATUS {
    HDCP_STATUS_AUTHENTICATED,            /**< HDCP Authentication Process is initiated and Passed */
    HDCP_STATUS_AUTHENTICATIONFAILURE,    /**< HDCP Authentication Failure or Link Integroty Failure */
    HDCP_STATUS_MAX                       /**< Maximum index for HDCP status. */
} ENUM_HDCP_STATUS;
typedef void (*displayEventCallback)(ENUM_DISPLAY_EVENT enEvent, void *eventData/*Optional*/);
bool  RegisterDisplayEventCallback(displayEventCallback cb);
void startDisplayUeventMonitor();
void stopDisplayUeventMonitor();

#if defined(__cplusplus)
}
#endif

#endif
