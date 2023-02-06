
#ifndef LTP_LIB_EXPORT_H
#define LTP_LIB_EXPORT_H

#ifdef LTP_LIB_STATIC_DEFINE
#  define LTP_LIB_EXPORT
#  define LTP_LIB_NO_EXPORT
#else
#  ifndef LTP_LIB_EXPORT
#    ifdef ltp_lib_EXPORTS
        /* We are building this library */
#      define LTP_LIB_EXPORT 
#    else
        /* We are using this library */
#      define LTP_LIB_EXPORT 
#    endif
#  endif

#  ifndef LTP_LIB_NO_EXPORT
#    define LTP_LIB_NO_EXPORT 
#  endif
#endif

#ifndef LTP_LIB_DEPRECATED
#  define LTP_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef LTP_LIB_DEPRECATED_EXPORT
#  define LTP_LIB_DEPRECATED_EXPORT LTP_LIB_EXPORT LTP_LIB_DEPRECATED
#endif

#ifndef LTP_LIB_DEPRECATED_NO_EXPORT
#  define LTP_LIB_DEPRECATED_NO_EXPORT LTP_LIB_NO_EXPORT LTP_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LTP_LIB_NO_DEPRECATED
#    define LTP_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* LTP_LIB_EXPORT_H */
