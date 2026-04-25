#ifndef XR_KINNOWFB_H
#define XR_KINNOWFB_H

#include "screen.h"

int KinnowInit(void);
void KinnowDraw(struct Screen *screen);
void KinnowDump(void);
void KinnowCreateScreen(void);

#endif // XR_KINNOWFB_H