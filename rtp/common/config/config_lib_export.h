
#ifndef CONFIG_LIB_EXPORT_H
#define CONFIG_LIB_EXPORT_H

#ifdef CONFIG_LIB_STATIC_DEFINE
#  define CONFIG_LIB_EXPORT
#  define CONFIG_LIB_NO_EXPORT
#else
#  ifndef CONFIG_LIB_EXPORT
#    ifdef config_lib_EXPORTS
        /* We are building this library */
#      define CONFIG_LIB_EXPORT 
#    else
        /* We are using this library */
#      define CONFIG_LIB_EXPORT 
#    endif
#  endif

#  ifndef CONFIG_LIB_NO_EXPORT
#    define CONFIG_LIB_NO_EXPORT 
#  endif
#endif

#ifndef CONFIG_LIB_DEPRECATED
#  define CONFIG_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef CONFIG_LIB_DEPRECATED_EXPORT
#  define CONFIG_LIB_DEPRECATED_EXPORT CONFIG_LIB_EXPORT CONFIG_LIB_DEPRECATED
#endif

#ifndef CONFIG_LIB_DEPRECATED_NO_EXPORT
#  define CONFIG_LIB_DEPRECATED_NO_EXPORT CONFIG_LIB_NO_EXPORT CONFIG_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef CONFIG_LIB_NO_DEPRECATED
#    define CONFIG_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* CONFIG_LIB_EXPORT_H */
