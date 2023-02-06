
#ifndef STORAGE_LIB_EXPORT_H
#define STORAGE_LIB_EXPORT_H

#ifdef STORAGE_LIB_STATIC_DEFINE
#  define STORAGE_LIB_EXPORT
#  define STORAGE_LIB_NO_EXPORT
#else
#  ifndef STORAGE_LIB_EXPORT
#    ifdef storage_lib_EXPORTS
        /* We are building this library */
#      define STORAGE_LIB_EXPORT 
#    else
        /* We are using this library */
#      define STORAGE_LIB_EXPORT 
#    endif
#  endif

#  ifndef STORAGE_LIB_NO_EXPORT
#    define STORAGE_LIB_NO_EXPORT 
#  endif
#endif

#ifndef STORAGE_LIB_DEPRECATED
#  define STORAGE_LIB_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef STORAGE_LIB_DEPRECATED_EXPORT
#  define STORAGE_LIB_DEPRECATED_EXPORT STORAGE_LIB_EXPORT STORAGE_LIB_DEPRECATED
#endif

#ifndef STORAGE_LIB_DEPRECATED_NO_EXPORT
#  define STORAGE_LIB_DEPRECATED_NO_EXPORT STORAGE_LIB_NO_EXPORT STORAGE_LIB_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef STORAGE_LIB_NO_DEPRECATED
#    define STORAGE_LIB_NO_DEPRECATED
#  endif
#endif

#endif /* STORAGE_LIB_EXPORT_H */
