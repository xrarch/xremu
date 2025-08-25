#ifndef XR_DEFS_H
#define XR_DEFS_H

#define XR_PRESERVE_NONE [[clang::preserve_none]]
#define XR_TAIL [[clang::musttail]]
#define XR_ALWAYS_INLINE __attribute__((always_inline))

#define XrLikely(x)       __builtin_expect(!!(x), 1)
#define XrUnlikely(x)     __builtin_expect(!!(x), 0)

#endif