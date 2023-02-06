
#ifndef STCP_LIB_EXPORT_H
#define STCP_LIB_EXPORT_H

#ifdef STCP_LIB_STATIC_DEFINE
#  define STCP_LIB_EXPORT
#  define STCP_LIB_NO_EXPORT
#else
#  ifndef STCP_LIB_EXPORT
#    ifdef stcp_lib_EXPORTS
        /* We are building this library */
#      define STCP_LIB_EXPORT 
#    else
        /* We are using this library */
#      define STCP_LIB_EXPORT 
#    endif
#  endif

#  ifndef STCP_LIB_NO_EXPORT
#    define STCP_LIB_NO_EXPORT 
#  endif
#endif

#ifndef STCP_LIB_DEPRECATED
#  define STCP_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef STCP_LIB_DEPRECATED_EXPORT
#  define STCP_LIB_DEPRECATED_EXPORT STCP_LIB_EXPORT STCP_LIB_DEPRECATED
#endif

#ifndef STCP_LIB_DEPRECATED_NO_EXPORT
#  define STCP_LIB_DEPRECATED_NO_EXPORT STCP_LIB_NO_EXPORT STCP_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef STCP_LIB_NO_DEPRECATED
#    define STCP_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* STCP_LIB_EXPORT_H */
