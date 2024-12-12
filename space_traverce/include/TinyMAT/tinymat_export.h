
#ifndef TINYMAT_EXPORT_H
#define TINYMAT_EXPORT_H

#define TINYMAT_STATIC_DEFINE

#ifdef TINYMAT_STATIC_DEFINE
#  define TINYMAT_EXPORT
#  define TINYMAT_NO_EXPORT
#else
#  ifndef TINYMAT_EXPORT
#    ifdef TinyMAT_EXPORTS
        /* We are building this library */
#      define TINYMAT_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define TINYMAT_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef TINYMAT_NO_EXPORT
#    define TINYMAT_NO_EXPORT 
#  endif
#endif

#ifndef TINYMAT_DEPRECATED
#  define TINYMAT_DEPRECATED __declspec(deprecated)
#endif

#ifndef TINYMAT_DEPRECATED_EXPORT
#  define TINYMAT_DEPRECATED_EXPORT TINYMAT_EXPORT TINYMAT_DEPRECATED
#endif

#ifndef TINYMAT_DEPRECATED_NO_EXPORT
#  define TINYMAT_DEPRECATED_NO_EXPORT TINYMAT_NO_EXPORT TINYMAT_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef TINYMAT_NO_DEPRECATED
#    define TINYMAT_NO_DEPRECATED
#  endif
#endif

#endif /* TINYMAT_EXPORT_H */
