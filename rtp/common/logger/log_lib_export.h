
#ifndef LOG_LIB_EXPORT_H
#define LOG_LIB_EXPORT_H

#ifdef LOG_LIB_STATIC_DEFINE
#  define LOG_LIB_EXPORT
#  define LOG_LIB_NO_EXPORT
#else
#  ifndef LOG_LIB_EXPORT
#    ifdef log_lib_EXPORTS
        /* We are building this library */
#      define LOG_LIB_EXPORT 
#    else
        /* We are using this library */
#      define LOG_LIB_EXPORT 
#    endif
#  endif

#  ifndef LOG_LIB_NO_EXPORT
#    define LOG_LIB_NO_EXPORT 
#  endif
#endif

#ifndef LOG_LIB_DEPRECATED
#  define LOG_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef LOG_LIB_DEPRECATED_EXPORT
#  define LOG_LIB_DEPRECATED_EXPORT LOG_LIB_EXPORT LOG_LIB_DEPRECATED
#endif

#ifndef LOG_LIB_DEPRECATED_NO_EXPORT
#  define LOG_LIB_DEPRECATED_NO_EXPORT LOG_LIB_NO_EXPORT LOG_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LOG_LIB_NO_DEPRECATED
#    define LOG_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* LOG_LIB_EXPORT_H */
