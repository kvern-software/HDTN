
#ifndef TCPCL_LIB_EXPORT_H
#define TCPCL_LIB_EXPORT_H

#ifdef TCPCL_LIB_STATIC_DEFINE
#  define TCPCL_LIB_EXPORT
#  define TCPCL_LIB_NO_EXPORT
#else
#  ifndef TCPCL_LIB_EXPORT
#    ifdef tcpcl_lib_EXPORTS
        /* We are building this library */
#      define TCPCL_LIB_EXPORT 
#    else
        /* We are using this library */
#      define TCPCL_LIB_EXPORT 
#    endif
#  endif

#  ifndef TCPCL_LIB_NO_EXPORT
#    define TCPCL_LIB_NO_EXPORT 
#  endif
#endif

#ifndef TCPCL_LIB_DEPRECATED
#  define TCPCL_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef TCPCL_LIB_DEPRECATED_EXPORT
#  define TCPCL_LIB_DEPRECATED_EXPORT TCPCL_LIB_EXPORT TCPCL_LIB_DEPRECATED
#endif

#ifndef TCPCL_LIB_DEPRECATED_NO_EXPORT
#  define TCPCL_LIB_DEPRECATED_NO_EXPORT TCPCL_LIB_NO_EXPORT TCPCL_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef TCPCL_LIB_NO_DEPRECATED
#    define TCPCL_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* TCPCL_LIB_EXPORT_H */
