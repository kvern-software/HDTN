
#ifndef STATS_LIB_EXPORT_H
#define STATS_LIB_EXPORT_H

#ifdef STATS_LIB_STATIC_DEFINE
#  define STATS_LIB_EXPORT
#  define STATS_LIB_NO_EXPORT
#else
#  ifndef STATS_LIB_EXPORT
#    ifdef stats_lib_EXPORTS
        /* We are building this library */
#      define STATS_LIB_EXPORT 
#    else
        /* We are using this library */
#      define STATS_LIB_EXPORT 
#    endif
#  endif

#  ifndef STATS_LIB_NO_EXPORT
#    define STATS_LIB_NO_EXPORT 
#  endif
#endif

#ifndef STATS_LIB_DEPRECATED
#  define STATS_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef STATS_LIB_DEPRECATED_EXPORT
#  define STATS_LIB_DEPRECATED_EXPORT STATS_LIB_EXPORT STATS_LIB_DEPRECATED
#endif

#ifndef STATS_LIB_DEPRECATED_NO_EXPORT
#  define STATS_LIB_DEPRECATED_NO_EXPORT STATS_LIB_NO_EXPORT STATS_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef STATS_LIB_NO_DEPRECATED
#    define STATS_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* STATS_LIB_EXPORT_H */
