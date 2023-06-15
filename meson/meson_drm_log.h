#ifndef LIBDRM_MESON_LOG_H_
#define LIBDRM_MESON_LOG_H_
#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void mesonDrmLog( int level, const char *fmt, ... );
long long getMonotonicTimeMicros( void );

#define INT_ERROR(FORMAT, ...)      mesonDrmLog(0, "ERROR: %s:%d " FORMAT "\n", __FILE__, __LINE__, __VA_ARGS__)
#define INT_INFO(FORMAT, ...)       mesonDrmLog(1, "INFO: %s:%d " FORMAT "\n", __FILE__, __LINE__, __VA_ARGS__)
#define INT_DEBUG(FORMAT, ...)      mesonDrmLog(2, "DEBUG: %s:%d " FORMAT "\n", __FILE__, __LINE__, __VA_ARGS__)

#define ERROR(...)                  INT_ERROR(__VA_ARGS__, "")
#define INFO(...)                   INT_INFO(__VA_ARGS__, "")
#define DEBUG(...)                  INT_DEBUG(__VA_ARGS__, "")

#if defined(__cplusplus)
}
#endif

#endif