#ifndef XR_DEFS_H
#define XR_DEFS_H

#include "stdint.h"

#ifndef EMSCRIPTEN
#define XR_PRESERVE_NONE [[clang::preserve_none]]
#else
#define XR_PRESERVE_NONE
#endif

#define XR_TAIL [[clang::musttail]]
#define XR_ALWAYS_INLINE __attribute__((always_inline))

#define XrLikely(x)       __builtin_expect(!!(x), 1)
#define XrUnlikely(x)     __builtin_expect(!!(x), 0)

typedef struct _XrNumaNode {
	uint32_t RamSize;
	uint32_t ProcessorCount;
} XrNumaNode;

#define XR_NODE_MAX 4
#define XR_PROC_PER_NODE_MAX 4

#define XR_PROC_MAX (XR_NODE_MAX * XR_PROC_PER_NODE_MAX)

extern XrNumaNode XrNumaNodes[XR_NODE_MAX];

#endif