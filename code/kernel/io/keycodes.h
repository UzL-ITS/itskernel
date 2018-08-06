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
	VKEY_LALT = 56,
	VKEY_SPACE = 57,
	VKEY_TAB = 58,
	VKEY_ENTER = 59,
	VKEY_BACKSPACE = 60,
	
	// Some special characters.
	VKEY_SEMICOLON = 61,
	VKEY_COMMA = 62,
	VKEY_DOT = 63,
	VKEY_MINUS = 64,
	VKEY_EQUAL = 65,
	VKEY_SQUOTE = 66,
	VKEY_BACKTICK = 67,
	VKEY_SLASH = 68,
	VKEY_BACKSLASH = 69,
	VKEY_LBRACKET = 70,
	VKEY_RBRACKET = 71,
	
	// Key pad.
	VKEY_KPMULTIPLY = 72,
	VKEY_KPMINUS = 73,
	VKEY_KPPLUS = 74,
	VKEY_KPDOT = 75,
	VKEY_KP0 = 76,
	VKEY_KP1 = 77,
	VKEY_KP2 = 78,
	VKEY_KP3 = 79,
	VKEY_KP4 = 80,
	VKEY_KP5 = 81,
	VKEY_KP6 = 82,
	VKEY_KP7 = 83,
	VKEY_KP8 = 84,
	VKEY_KP9 = 85,
	
	// Dummy element to safely retrieve maximum virtual key code
	VKEY_MAX_VALUE = VKEY_KP9
} vkey_t;

// Conversion table for one-byte scan codes to virtual key codes.
extern vkey_t oneByteScanCodeConversionTable[];

// Conversion table for E0-two-byte scan codes to virtual key codes.
extern vkey_t e0ScanCodeConversionTable[];