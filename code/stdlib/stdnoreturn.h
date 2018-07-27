
#ifndef STDNORETURN_H
#define STDNORETURN_H

#if defined(__clang__) && __clang_major__ == 3 && __clang_minor__ == 0
#define noreturn
#elif !defined(__OPTIMIZE__)
/*
 * _Noreturn confuses stack traces as the CALL places RIP+n on the stack (where
 * n is the number of bytes of the encoded CALL instruction). However, as gcc
 * optimizes away the function's exit routine, this actually makes RIP+n be the
 * start of the _following_ function, making us show the incorrect symbol name.
 *
 * Therefore if optimization is disabled we don't add _Noreturn to functions,
 * which makes debugging easier.
 */
#define noreturn
#else
#define noreturn _Noreturn
#endif

#endif
