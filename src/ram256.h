#ifndef XR_RAM256_H
#define	XR_RAM256_H

#include <stdint.h>
#include "xrdefs.h"

#define RAMSLOTSIZE (32 * 1024 * 1024)
#define RAMSLOTCOUNT 8
#define RAMMAXIMUM (RAMSLOTSIZE * RAMSLOTCOUNT)

#define SLOTS_PER_NODE (RAMSLOTCOUNT / XR_NODE_MAX)
#define RAM_PER_NODE (SLOTS_PER_NODE * RAMSLOTSIZE)

extern int RAMInit();
extern void RAMDump();

#endif // XR_RAM256_H