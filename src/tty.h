struct TTY;

typedef void (*TTYInputF)(struct TTY *tty, uint16_t c);

#define TTYPARAMCOUNT 4

struct TTY {
	uint16_t *TextBuffer;
	uint32_t Width;
	uint32_t Height;

	int CursorX;
	int CursorY;

	int DirtyX1;
	int DirtyY1;
	int DirtyX2;
	int DirtyY2;
	int IsDirty;

	int IsCtrl;
	int IsShift;
	int IsEscape;

	int IsCursorHidden;

	int EscapeIndex;
	uint32_t EscapeParams[TTYPARAMCOUNT];

	void *Context;

	uint32_t ScrollWindowTop;
	uint32_t ScrollWindowBottom;

	int CurrentAttributes;

	TTYInputF Input;

	struct Screen *Screen;
};

struct TTY *TTYCreate(int width, int height, char *title, TTYInputF input);

void TTYPutCharacter(struct TTY *tty, char c);

static uint8_t KeyMapNormal[SDL_NUM_SCANCODES] = {
	[SDL_SCANCODE_A] = 'a',
	[SDL_SCANCODE_B] = 'b',
	[SDL_SCANCODE_C] = 'c',
	[SDL_SCANCODE_D] = 'd',
	[SDL_SCANCODE_E] = 'e',
	[SDL_SCANCODE_F] = 'f',
	[SDL_SCANCODE_G] = 'g',
	[SDL_SCANCODE_H] = 'h',
	[SDL_SCANCODE_I] = 'i',
	[SDL_SCANCODE_J] = 'j',
	[SDL_SCANCODE_K] = 'k',
	[SDL_SCANCODE_L] = 'l',
	[SDL_SCANCODE_M] = 'm',
	[SDL_SCANCODE_N] = 'n',
	[SDL_SCANCODE_O] = 'o',
	[SDL_SCANCODE_P] = 'p',
	[SDL_SCANCODE_Q] = 'q',
	[SDL_SCANCODE_R] = 'r',
	[SDL_SCANCODE_S] = 's',
	[SDL_SCANCODE_T] = 't',
	[SDL_SCANCODE_U] = 'u',
	[SDL_SCANCODE_V] = 'v',
	[SDL_SCANCODE_W] = 'w',
	[SDL_SCANCODE_X] = 'x',
	[SDL_SCANCODE_Y] = 'y',
	[SDL_SCANCODE_Z] = 'z',

	[SDL_SCANCODE_0] = '0',
	[SDL_SCANCODE_1] = '1',
	[SDL_SCANCODE_2] = '2',
	[SDL_SCANCODE_3] = '3',
	[SDL_SCANCODE_4] = '4',
	[SDL_SCANCODE_5] = '5',
	[SDL_SCANCODE_6] = '6',
	[SDL_SCANCODE_7] = '7',
	[SDL_SCANCODE_8] = '8',
	[SDL_SCANCODE_9] = '9',

	[SDL_SCANCODE_SEMICOLON] = ';',
	[SDL_SCANCODE_SPACE]     = ' ',
	[SDL_SCANCODE_TAB]       = '\t',

	[SDL_SCANCODE_MINUS]        = '-',
	[SDL_SCANCODE_EQUALS]       = '=',
	[SDL_SCANCODE_LEFTBRACKET]  = '[',
	[SDL_SCANCODE_RIGHTBRACKET] = ']',
	[SDL_SCANCODE_BACKSLASH]    = '\\',
	[SDL_SCANCODE_NONUSHASH]    = '\\',  // same key as BACKSLASH
	[SDL_SCANCODE_SLASH]      = '/',
	[SDL_SCANCODE_PERIOD]     = '.',
	[SDL_SCANCODE_APOSTROPHE] = '\'',
	[SDL_SCANCODE_COMMA]      = ',',
	[SDL_SCANCODE_GRAVE]      = '`',
	[SDL_SCANCODE_RETURN]    = '\r',
	[SDL_SCANCODE_BACKSPACE] = '\b',
	[SDL_SCANCODE_CAPSLOCK]  = -1,
	[SDL_SCANCODE_ESCAPE]    = -1,

	[SDL_SCANCODE_LEFT]     = -1,
	[SDL_SCANCODE_RIGHT]    = -1,
	[SDL_SCANCODE_DOWN]     = -1,
	[SDL_SCANCODE_UP]       = -1,

	[SDL_SCANCODE_LALT]   = -1,
	[SDL_SCANCODE_RALT]   = -1,

	[SDL_SCANCODE_KP_DIVIDE]   = '\\',
	[SDL_SCANCODE_KP_MINUS]    = '-',
	[SDL_SCANCODE_KP_ENTER]    = '\n',
	[SDL_SCANCODE_KP_0]        = '0',
	[SDL_SCANCODE_KP_1]        = '1',
	[SDL_SCANCODE_KP_2]        = '2',
	[SDL_SCANCODE_KP_3]        = '3',
	[SDL_SCANCODE_KP_4]        = '4',
	[SDL_SCANCODE_KP_5]        = '5',
	[SDL_SCANCODE_KP_6]        = '6',
	[SDL_SCANCODE_KP_7]        = '7',
	[SDL_SCANCODE_KP_8]        = '8',
	[SDL_SCANCODE_KP_9]        = '9',
	[SDL_SCANCODE_KP_PERIOD]   = '.',
};

static uint8_t KeyMapCtrl[SDL_NUM_SCANCODES] = {
	[SDL_SCANCODE_A] = 1,
	[SDL_SCANCODE_B] = 2,
	[SDL_SCANCODE_C] = 3,
	[SDL_SCANCODE_D] = 4,
	[SDL_SCANCODE_E] = 5,
	[SDL_SCANCODE_F] = 6,
	[SDL_SCANCODE_G] = 7,
	[SDL_SCANCODE_H] = 8,
	[SDL_SCANCODE_I] = 9,
	[SDL_SCANCODE_J] = 10,
	[SDL_SCANCODE_K] = 11,
	[SDL_SCANCODE_L] = 12,
	[SDL_SCANCODE_M] = 13,
	[SDL_SCANCODE_N] = 14,
	[SDL_SCANCODE_O] = 15,
	[SDL_SCANCODE_P] = 16,
	[SDL_SCANCODE_Q] = 17,
	[SDL_SCANCODE_R] = 18,
	[SDL_SCANCODE_S] = 19,
	[SDL_SCANCODE_T] = 20,
	[SDL_SCANCODE_U] = 21,
	[SDL_SCANCODE_V] = 22,
	[SDL_SCANCODE_W] = 23,
	[SDL_SCANCODE_X] = 24,
	[SDL_SCANCODE_Y] = 25,
	[SDL_SCANCODE_Z] = 26,

	[SDL_SCANCODE_0] = -1,
	[SDL_SCANCODE_1] = -1,
	[SDL_SCANCODE_2] = 0,
	[SDL_SCANCODE_3] = -1,
	[SDL_SCANCODE_4] = -1,
	[SDL_SCANCODE_5] = -1,
	[SDL_SCANCODE_6] = 30,
	[SDL_SCANCODE_7] = -1,
	[SDL_SCANCODE_8] = -1,
	[SDL_SCANCODE_9] = -1,

	[SDL_SCANCODE_SEMICOLON] = -1,
	[SDL_SCANCODE_SPACE]     = -1,
	[SDL_SCANCODE_TAB]       = -1,

	[SDL_SCANCODE_MINUS]        = 31,
	[SDL_SCANCODE_EQUALS]       = -1,
	[SDL_SCANCODE_LEFTBRACKET]  = -1,
	[SDL_SCANCODE_RIGHTBRACKET] = 29,
	[SDL_SCANCODE_BACKSLASH]    = 28,
	[SDL_SCANCODE_NONUSHASH]    = 28,  // same key as BACKSLASH
	[SDL_SCANCODE_SLASH]      = -1,
	[SDL_SCANCODE_PERIOD]     = -1,
	[SDL_SCANCODE_APOSTROPHE] = -1,
	[SDL_SCANCODE_COMMA]      = -1,
	[SDL_SCANCODE_GRAVE]      = -1,
	[SDL_SCANCODE_RETURN]    = -1,
	[SDL_SCANCODE_BACKSPACE] = -1,
	[SDL_SCANCODE_CAPSLOCK]  = -1,
	[SDL_SCANCODE_ESCAPE]    = -1,

	[SDL_SCANCODE_LEFT]     = -1,
	[SDL_SCANCODE_RIGHT]    = -1,
	[SDL_SCANCODE_DOWN]     = -1,
	[SDL_SCANCODE_UP]       = -1,

	[SDL_SCANCODE_LALT]   = -1,
	[SDL_SCANCODE_RALT]   = -1,

	[SDL_SCANCODE_KP_DIVIDE]   = 28,
	[SDL_SCANCODE_KP_MINUS]    = 31,
	[SDL_SCANCODE_KP_ENTER]    = -1,
	[SDL_SCANCODE_KP_0]        = -1,
	[SDL_SCANCODE_KP_1]        = -1,
	[SDL_SCANCODE_KP_2]        = 0,
	[SDL_SCANCODE_KP_3]        = -1,
	[SDL_SCANCODE_KP_4]        = -1,
	[SDL_SCANCODE_KP_5]        = -1,
	[SDL_SCANCODE_KP_6]        = 30,
	[SDL_SCANCODE_KP_7]        = -1,
	[SDL_SCANCODE_KP_8]        = -1,
	[SDL_SCANCODE_KP_9]        = -1,
	[SDL_SCANCODE_KP_PERIOD]   = -1,
};

static uint8_t KeyMapShift[SDL_NUM_SCANCODES] = {
	[SDL_SCANCODE_A] = 'A',
	[SDL_SCANCODE_B] = 'B',
	[SDL_SCANCODE_C] = 'C',
	[SDL_SCANCODE_D] = 'D',
	[SDL_SCANCODE_E] = 'E',
	[SDL_SCANCODE_F] = 'F',
	[SDL_SCANCODE_G] = 'G',
	[SDL_SCANCODE_H] = 'H',
	[SDL_SCANCODE_I] = 'I',
	[SDL_SCANCODE_J] = 'J',
	[SDL_SCANCODE_K] = 'K',
	[SDL_SCANCODE_L] = 'L',
	[SDL_SCANCODE_M] = 'M',
	[SDL_SCANCODE_N] = 'N',
	[SDL_SCANCODE_O] = 'O',
	[SDL_SCANCODE_P] = 'P',
	[SDL_SCANCODE_Q] = 'Q',
	[SDL_SCANCODE_R] = 'R',
	[SDL_SCANCODE_S] = 'S',
	[SDL_SCANCODE_T] = 'T',
	[SDL_SCANCODE_U] = 'U',
	[SDL_SCANCODE_V] = 'V',
	[SDL_SCANCODE_W] = 'W',
	[SDL_SCANCODE_X] = 'X',
	[SDL_SCANCODE_Y] = 'Y',
	[SDL_SCANCODE_Z] = 'Z',

	[SDL_SCANCODE_0] = ')',
	[SDL_SCANCODE_1] = '!',
	[SDL_SCANCODE_2] = '@',
	[SDL_SCANCODE_3] = '#',
	[SDL_SCANCODE_4] = '$',
	[SDL_SCANCODE_5] = '%',
	[SDL_SCANCODE_6] = '^',
	[SDL_SCANCODE_7] = '&',
	[SDL_SCANCODE_8] = '*',
	[SDL_SCANCODE_9] = '(',

	[SDL_SCANCODE_SEMICOLON] = ':',
	[SDL_SCANCODE_SPACE]     = ' ',
	[SDL_SCANCODE_TAB]       = '\t',

	[SDL_SCANCODE_MINUS]        = '_',
	[SDL_SCANCODE_EQUALS]       = '+',
	[SDL_SCANCODE_LEFTBRACKET]  = '{',
	[SDL_SCANCODE_RIGHTBRACKET] = '}',
	[SDL_SCANCODE_BACKSLASH]    = '|',
	[SDL_SCANCODE_NONUSHASH]    = '|',  // same key as BACKSLASH
	[SDL_SCANCODE_SLASH]      = '?',
	[SDL_SCANCODE_PERIOD]     = '>',
	[SDL_SCANCODE_APOSTROPHE] = '"',
	[SDL_SCANCODE_COMMA]      = '<',
	[SDL_SCANCODE_GRAVE]      = '~',
	[SDL_SCANCODE_RETURN]    = '\n',
	[SDL_SCANCODE_BACKSPACE] = '\b',
	[SDL_SCANCODE_CAPSLOCK]  = -1,
	[SDL_SCANCODE_ESCAPE]    = -1,

	[SDL_SCANCODE_LEFT]     = -1,
	[SDL_SCANCODE_RIGHT]    = -1,
	[SDL_SCANCODE_DOWN]     = -1,
	[SDL_SCANCODE_UP]       = -1,

	[SDL_SCANCODE_LALT]   = -1,
	[SDL_SCANCODE_RALT]   = -1,

	[SDL_SCANCODE_KP_DIVIDE]   = '\\',
	[SDL_SCANCODE_KP_MINUS]    = '-',
	[SDL_SCANCODE_KP_ENTER]    = '\n',
	[SDL_SCANCODE_KP_0]        = '0',
	[SDL_SCANCODE_KP_1]        = '1',
	[SDL_SCANCODE_KP_2]        = '2',
	[SDL_SCANCODE_KP_3]        = '3',
	[SDL_SCANCODE_KP_4]        = '4',
	[SDL_SCANCODE_KP_5]        = '5',
	[SDL_SCANCODE_KP_6]        = '6',
	[SDL_SCANCODE_KP_7]        = '7',
	[SDL_SCANCODE_KP_8]        = '8',
	[SDL_SCANCODE_KP_9]        = '9',
	[SDL_SCANCODE_KP_PERIOD]   = '.',
};