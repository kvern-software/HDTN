
#ifndef CGR_LIB_EXPORT_H
#define CGR_LIB_EXPORT_H

#ifdef CGR_LIB_STATIC_DEFINE
#  define CGR_LIB_EXPORT
#  define CGR_LIB_NO_EXPORT
#else
#  ifndef CGR_LIB_EXPORT
#    ifdef cgr_lib_EXPORTS
        /* We are building this library */
#      define CGR_LIB_EXPORT 
#    else
        /* We are using this library */
#      define CGR_LIB_EXPORT 
#    endif
#  endif

#  ifndef CGR_LIB_NO_EXPORT
#    define CGR_LIB_NO_EXPORT 
#  endif
#endif

#ifndef CGR_LIB_DEPRECATED
#  define CGR_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef CGR_LIB_DEPRECATED_EXPORT
#  define CGR_LIB_DEPRECATED_EXPORT CGR_LIB_EXPORT CGR_LIB_DEPRECATED
#endif

#ifndef CGR_LIB_DEPRECATED_NO_EXPORT
#  define CGR_LIB_DEPRECATED_NO_EXPORT CGR_LIB_NO_EXPORT CGR_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef CGR_LIB_NO_DEPRECATED
#    define CGR_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* CGR_LIB_EXPORT_H */
