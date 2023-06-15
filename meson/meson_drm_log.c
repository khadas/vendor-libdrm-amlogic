#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "meson_drm_log.h"
#include <sys/time.h>

static int g_activeLevel= 1;

long long getMonotonicTimeMicros( void )
{
   int rc;
   struct timespec tm;
   long long timeMicro;
   static bool reportedError= false;
   if ( !reportedError )
   {
      rc= clock_gettime( CLOCK_MONOTONIC, &tm );
   }
   if ( reportedError || rc )
   {
      struct timeval tv;
      if ( !reportedError )
      {
         reportedError= true;
         ERROR("clock_gettime failed rc %d - using timeofday", rc);
      }
      gettimeofday(&tv,0);
      timeMicro= tv.tv_sec*1000000LL+tv.tv_usec;
   }
   else
   {
      timeMicro= tm.tv_sec*1000000LL+(tm.tv_nsec/1000LL);
   }
   return timeMicro;
}

void mesonDrmLog( int level, const char *fmt, ... )
{
   const char *env= getenv( "LIBMESON_GL_DEBUG" );
   if ( env )
   {
      int level= atoi( env );
      g_activeLevel= level;
   }
   if ( level <= g_activeLevel )
   {
      va_list argptr;
      fprintf( stderr, "%lld: ", getMonotonicTimeMicros());
      va_start( argptr, fmt );
      vfprintf( stderr, fmt, argptr );
      va_end( argptr );
   }
}
