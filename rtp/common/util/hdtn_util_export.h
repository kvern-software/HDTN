
#ifndef HDTN_UTIL_EXPORT_H
#define HDTN_UTIL_EXPORT_H

#ifdef HDTN_UTIL_STATIC_DEFINE
#  define HDTN_UTIL_EXPORT
#  define HDTN_UTIL_NO_EXPORT
#else
#  ifndef HDTN_UTIL_EXPORT
#    ifdef hdtn_util_EXPORTS
        /* We are building this library */
#      define HDTN_UTIL_EXPORT 
#    else
        /* We are using this library */
#      define HDTN_UTIL_EXPORT 
#    endif
#  endif

#  ifndef HDTN_UTIL_NO_EXPORT
#    define HDTN_UTIL_NO_EXPORT 
#  endif
#endif

#ifndef HDTN_UTIL_DEPRECATED
#  define HDTN_UTIL_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef HDTN_UTIL_DEPRECATED_EXPORT
#  define HDTN_UTIL_DEPRECATED_EXPORT HDTN_UTIL_EXPORT HDTN_UTIL_DEPRECATED
#endif

#ifndef HDTN_UTIL_DEPRECATED_NO_EXPORT
#  define HDTN_UTIL_DEPRECATED_NO_EXPORT HDTN_UTIL_NO_EXPORT HDTN_UTIL_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef HDTN_UTIL_NO_DEPRECATED
#    define HDTN_UTIL_NO_DEPRECATED
#  endif
#endif

#endif /* HDTN_UTIL_EXPORT_H */
