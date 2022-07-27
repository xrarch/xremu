struct TTY {
	uint16_t *TextBuffer;
	int Width;
	int Height;

	int CursorX;
	int CursorY;

	struct Screen *Screen;
};

struct TTY *TTYCreate(int width, int height, char *title);