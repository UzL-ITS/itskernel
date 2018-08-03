
#include <sys/syscalls.h>
#include <sys/msg.h>

uint64_t printf(const char *str)
{
	return testprintf(str);
}

uint8_t receive_keypress()
{
	printf("LIB: Receive enter.\n");
	msg_type_t newMsgType;
	do
		newMsgType = next_message_type();
	while(newMsgType == MSG_INVALID);
	printf("LIB: Received msg.\n");
	
	msg_key_press_t msg;
	next_message(&msg.header);
	return msg.scanCode;
}

void show_process(int contextId)
{
	printf("LIB: Trying to switch VBE context\n");
	set_displayed_process(contextId);
	printf("LIB: VBE context switch complete.\n");
}