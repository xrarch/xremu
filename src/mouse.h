#include "screen.h"

void MouseInit();

void MousePressed(struct Screen *screen, int button);

void MouseReleased(struct Screen *screen, int button);

void MouseMoved(struct Screen *screen, int dx, int dy);