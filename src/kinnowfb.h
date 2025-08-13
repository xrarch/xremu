#ifndef XR_KINNOWFB_H
#define XR_KINNOWFB_H

#include "screen.h"

#define KINNOW_FRAMEBUFFER_WIDTH  1024
#define KINNOW_FRAMEBUFFER_HEIGHT 768

int KinnowInit();
void KinnowDraw(struct Screen *screen);
void KinnowDump();

#endif // XR_KINNOWFB_H