#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"

void TextBlitCharacter(uint8_t c,
						uint8_t *fontbmp,
						int fontwidth,
						int fontheight,
						int x,
						int y,
						int destwidth,
						uint32_t fgcolor,
						uint32_t bgcolor,
						uint32_t *pixelbuffer) {

	int fontpitch = (fontwidth+7)/8;

	pixelbuffer += y*destwidth+x;

	fontbmp += c*fontpitch*fontheight;

	for (; fontheight; fontheight--) {
		for (int cx = 0; cx<fontwidth; cx++) {
			pixelbuffer[cx] = ((fontbmp[cx/8]>>(7-(cx&7))) & 1) ? fgcolor : bgcolor;
		}

		pixelbuffer += destwidth;
		fontbmp += fontpitch;
	}
}