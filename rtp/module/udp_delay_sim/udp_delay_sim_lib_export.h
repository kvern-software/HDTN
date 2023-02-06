
#ifndef UDP_DELAY_SIM_LIB_EXPORT_H
#define UDP_DELAY_SIM_LIB_EXPORT_H

#ifdef UDP_DELAY_SIM_LIB_STATIC_DEFINE
#  define UDP_DELAY_SIM_LIB_EXPORT
#  define UDP_DELAY_SIM_LIB_NO_EXPORT
#else
#  ifndef UDP_DELAY_SIM_LIB_EXPORT
#    ifdef udp_delay_sim_lib_EXPORTS
        /* We are building this library */
#      define UDP_DELAY_SIM_LIB_EXPORT 
#    else
        /* We are using this library */
#      define UDP_DELAY_SIM_LIB_EXPORT 
#    endif
#  endif

#  ifndef UDP_DELAY_SIM_LIB_NO_EXPORT
#    define UDP_DELAY_SIM_LIB_NO_EXPORT 
#  endif
#endif

#ifndef UDP_DELAY_SIM_LIB_DEPRECATED
#  define UDP_DELAY_SIM_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef UDP_DELAY_SIM_LIB_DEPRECATED_EXPORT
#  define UDP_DELAY_SIM_LIB_DEPRECATED_EXPORT UDP_DELAY_SIM_LIB_EXPORT UDP_DELAY_SIM_LIB_DEPRECATED
#endif

#ifndef UDP_DELAY_SIM_LIB_DEPRECATED_NO_EXPORT
#  define UDP_DELAY_SIM_LIB_DEPRECATED_NO_EXPORT UDP_DELAY_SIM_LIB_NO_EXPORT UDP_DELAY_SIM_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef UDP_DELAY_SIM_LIB_NO_DEPRECATED
#    define UDP_DELAY_SIM_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* UDP_DELAY_SIM_LIB_EXPORT_H */
