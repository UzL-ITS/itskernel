
#include <panic/panic.h>

#define static_assert _Static_assert

#ifdef NDEBUG
#define assert(expr) ((void) 0)
#else
#define assert(expr) ((expr) ? ((void) 0) : panic("%s:%d: %s: Assertion `%s' failed", __FILE__, __LINE__, __func__, #expr))
#endif
