/*
Kernel UI process main file.
*/

#include <libtest.h>


#define KEYBOARD_SCAN_CODE_F1 0x3B
#define KEYBOARD_SCAN_CODE_F2 0x3C
#define KEYBOARD_SCAN_CODE_F3 0x3D
#define KEYBOARD_SCAN_CODE_F4 0x3E
#define KEYBOARD_SCAN_CODE_F5 0x3F
#define KEYBOARD_SCAN_CODE_F6 0x40
#define KEYBOARD_SCAN_CODE_F7 0x41
#define KEYBOARD_SCAN_CODE_F8 0x42
#define KEYBOARD_SCAN_CODE_F9 0x43
#define KEYBOARD_SCAN_CODE_F10 0x44
#define KEYBOARD_SCAN_CODE_F11 0x57
#define KEYBOARD_SCAN_CODE_F12 0x58


int main()
{
	printf("hallo!!!!");
	
	while(1)
	{
		uint8_t scanCode = receive_keypress();
		if(KEYBOARD_SCAN_CODE_F1 <= scanCode && scanCode <= KEYBOARD_SCAN_CODE_F10)
			show_process(scanCode - KEYBOARD_SCAN_CODE_F1);
		else if(scanCode == KEYBOARD_SCAN_CODE_F11)
			show_process(10);
		else if(scanCode == KEYBOARD_SCAN_CODE_F12)
			show_process(11);
	}
	
	
	return 0;
}