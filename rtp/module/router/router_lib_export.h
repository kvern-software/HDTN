
#ifndef ROUTER_LIB_EXPORT_H
#define ROUTER_LIB_EXPORT_H

#ifdef ROUTER_LIB_STATIC_DEFINE
#  define ROUTER_LIB_EXPORT
#  define ROUTER_LIB_NO_EXPORT
#else
#  ifndef ROUTER_LIB_EXPORT
#    ifdef router_lib_EXPORTS
        /* We are building this library */
#      define ROUTER_LIB_EXPORT 
#    else
        /* We are using this library */
#      define ROUTER_LIB_EXPORT 
#    endif
#  endif

#  ifndef ROUTER_LIB_NO_EXPORT
#    define ROUTER_LIB_NO_EXPORT 
#  endif
#endif

#ifndef ROUTER_LIB_DEPRECATED
#  define ROUTER_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef ROUTER_LIB_DEPRECATED_EXPORT
#  define ROUTER_LIB_DEPRECATED_EXPORT ROUTER_LIB_EXPORT ROUTER_LIB_DEPRECATED
#endif

#ifndef ROUTER_LIB_DEPRECATED_NO_EXPORT
#  define ROUTER_LIB_DEPRECATED_NO_EXPORT ROUTER_LIB_NO_EXPORT ROUTER_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ROUTER_LIB_NO_DEPRECATED
#    define ROUTER_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* ROUTER_LIB_EXPORT_H */
