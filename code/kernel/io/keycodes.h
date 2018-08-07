#pragma once

/*
ITS kernel virtual key codes.
*/

// Virtual key codes.
typedef enum
{
	// Invalid key.
	VKEY_INVALID = 0,
	
	// Alphabetic keys.
	VKEY_A = 1,
	VKEY_B = 2,
	VKEY_C = 3,
	VKEY_D = 4,
	VKEY_E = 5,
	VKEY_F = 6,
	VKEY_G = 7,
	VKEY_H = 8,
	VKEY_I = 9,
	VKEY_J = 10,
	VKEY_K = 11,
	VKEY_L = 12,
	VKEY_M = 13,
	VKEY_N = 14,
	VKEY_O = 15,
	VKEY_P = 16,
	VKEY_Q = 17,
	VKEY_R = 18,
	VKEY_S = 19,
	VKEY_T = 20,
	VKEY_U = 21,
	VKEY_V = 22,
	VKEY_W = 23,
	VKEY_X = 24,
	VKEY_Y = 25,
	VKEY_Z = 26,
	
	// Numeric keys (on top of the alphabetic ones).
	VKEY_0 = 27,
	VKEY_1 = 28,
	VKEY_2 = 29,
	VKEY_3 = 30,
	VKEY_4 = 31,
	VKEY_5 = 32,
	VKEY_6 = 33,
	VKEY_7 = 34,
	VKEY_8 = 35,
	VKEY_9 = 36,
	
	// Fn keys.
	VKEY_F1 = 37,
	VKEY_F2 = 38,
	VKEY_F3 = 39,
	VKEY_F4 = 40,
	VKEY_F5 = 41,
	VKEY_F6 = 42,
	VKEY_F7 = 43,
	VKEY_F8 = 44,
	VKEY_F9 = 45,
	VKEY_F10 = 46,
	VKEY_F11 = 47,
	VKEY_F12 = 48,
	
	// Various control keys.
	VKEY_ESC = 49,
	VKEY_CAPSLOCK = 50,
	VKEY_NUMLOCK = 51,
	VKEY_SCRLOCK = 52,
	VKEY_LSHIFT = 53,
	VKEY_RSHIFT = 54,
	VKEY_LCONTROL = 55,
	VKEY_RCONTROL = 56,
	VKEY_LALT = 57,
	VKEY_RALT = 58,
	VKEY_LGUI = 59,
	VKEY_RGUI = 60,
	VKEY_APPS = 61,
	VKEY_SPACE = 62,
	VKEY_TAB = 63,
	VKEY_ENTER = 64,
	VKEY_BACKSPACE = 65,
	VKEY_INSERT = 66,
	VKEY_DELETE = 67,
	
	// Some special characters.
	VKEY_SEMICOLON = 68,
	VKEY_COMMA = 69,
	VKEY_DOT = 70,
	VKEY_MINUS = 71,
	VKEY_EQUAL = 72,
	VKEY_SQUOTE = 73,
	VKEY_BACKTICK = 74,
	VKEY_SLASH = 75,
	VKEY_BACKSLASH = 76,
	VKEY_LBRACKET = 77,
	VKEY_RBRACKET = 78,
	
	// Key pad.
	VKEY_KPMULTIPLY = 79,
	VKEY_KPDIV = 80,
	VKEY_KPMINUS = 81,
	VKEY_KPPLUS = 82,
	VKEY_KPDOT = 83,
	VKEY_KP0 = 84,
	VKEY_KP1 = 85,
	VKEY_KP2 = 86,
	VKEY_KP3 = 87,
	VKEY_KP4 = 88,
	VKEY_KP5 = 89,
	VKEY_KP6 = 90,
	VKEY_KP7 = 91,
	VKEY_KP8 = 92,
	VKEY_KP9 = 93,
	VKEY_KPENTER = 94,
	
	// Navigation keys.
	VKEY_CURSORUP = 95,
	VKEY_CURSORDOWN = 96,
	VKEY_CURSORLEFT = 97,
	VKEY_CURSORRIGHT = 98,
	VKEY_PAGEUP = 99,
	VKEY_PAGEDOWN = 100,
	VKEY_HOME = 101,
	VKEY_END = 102,
	
	// Multimedia keys.
	VKEY_MMPREVTRACK = 103,
	VKEY_MMNEXTTRACK = 104,
	VKEY_MMMUTE = 105,
	VKEY_MMCALC = 106,
	VKEY_MMPLAY = 107,
	VKEY_MMVOLDOWN = 108,
	VKEY_MMVOLUP = 109,
	VKEY_MMWWW = 110,
	
	// Power keys.
	VKEY_ACPIPOWER = 111,
	VKEY_ACPISLEEP = 112,
	VKEY_ACPIWAKE = 113,
	
	// Dummy element to safely retrieve maximum virtual key code
	VKEY_MAX_VALUE = VKEY_ACPIWAKE
} vkey_t;

// Conversion table for one-byte scan codes to virtual key codes.
extern vkey_t oneByteScanCodeConversionTable[];

// Conversion table for E0-two-byte scan codes to virtual key codes.
extern vkey_t e0ScanCodeConversionTable[];