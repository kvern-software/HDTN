
#ifndef SCHEDULER_LIB_EXPORT_H
#define SCHEDULER_LIB_EXPORT_H

#ifdef SCHEDULER_LIB_STATIC_DEFINE
#  define SCHEDULER_LIB_EXPORT
#  define SCHEDULER_LIB_NO_EXPORT
#else
#  ifndef SCHEDULER_LIB_EXPORT
#    ifdef scheduler_lib_EXPORTS
        /* We are building this library */
#      define SCHEDULER_LIB_EXPORT 
#    else
        /* We are using this library */
#      define SCHEDULER_LIB_EXPORT 
#    endif
#  endif

#  ifndef SCHEDULER_LIB_NO_EXPORT
#    define SCHEDULER_LIB_NO_EXPORT 
#  endif
#endif

#ifndef SCHEDULER_LIB_DEPRECATED
#  define SCHEDULER_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef SCHEDULER_LIB_DEPRECATED_EXPORT
#  define SCHEDULER_LIB_DEPRECATED_EXPORT SCHEDULER_LIB_EXPORT SCHEDULER_LIB_DEPRECATED
#endif

#ifndef SCHEDULER_LIB_DEPRECATED_NO_EXPORT
#  define SCHEDULER_LIB_DEPRECATED_NO_EXPORT SCHEDULER_LIB_NO_EXPORT SCHEDULER_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef SCHEDULER_LIB_NO_DEPRECATED
#    define SCHEDULER_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* SCHEDULER_LIB_EXPORT_H */
