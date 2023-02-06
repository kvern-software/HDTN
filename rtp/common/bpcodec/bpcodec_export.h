
#ifndef BPCODEC_EXPORT_H
#define BPCODEC_EXPORT_H

#ifdef BPCODEC_STATIC_DEFINE
#  define BPCODEC_EXPORT
#  define BPCODEC_NO_EXPORT
#else
#  ifndef BPCODEC_EXPORT
#    ifdef bpcodec_EXPORTS
        /* We are building this library */
#      define BPCODEC_EXPORT 
#    else
        /* We are using this library */
#      define BPCODEC_EXPORT 
#    endif
#  endif

#  ifndef BPCODEC_NO_EXPORT
#    define BPCODEC_NO_EXPORT 
#  endif
#endif

#ifndef BPCODEC_DEPRECATED
#  define BPCODEC_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef BPCODEC_DEPRECATED_EXPORT
#  define BPCODEC_DEPRECATED_EXPORT BPCODEC_EXPORT BPCODEC_DEPRECATED
#endif

#ifndef BPCODEC_DEPRECATED_NO_EXPORT
#  define BPCODEC_DEPRECATED_NO_EXPORT BPCODEC_NO_EXPORT BPCODEC_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef BPCODEC_NO_DEPRECATED
#    define BPCODEC_NO_DEPRECATED
#  endif
#endif

#endif /* BPCODEC_EXPORT_H */
