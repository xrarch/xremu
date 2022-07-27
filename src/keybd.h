#include "screen.h"

void KeyboardInit();

#define KEYBDMAXCODE 85

void KeyboardPressed(struct Screen *screen, int sdlscancode);

void KeyboardReleased(struct Screen *screen, int sdlscancode);