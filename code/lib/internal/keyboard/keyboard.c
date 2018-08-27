/*
ITS kernel keyboard handling.

This code starts a separate thread that frequently polls the kernel for new key presses
and stores these within a FIFO queue. The queue is implemented as a ring buffer which size
is doubled each time it overflows.
*/

/* INCLUDES */

#include <internal/keyboard/keyboard.h>
#include <internal/syscall/syscalls.h>
#include <threading/thread.h>
#include <threading/lock.h>
#include <stdint.h>
#include <memory.h>


/* TYPES */

// Queue element type.
typedef struct
{
	// Key code.
	vkey_t keyCode;
	
	// Shift key pressed flag.
	bool shiftPressed;
	
} queue_element_t;


/* VARIABLES */

// Mutex for accessing the internal key code queue.
static mutex_t queueMutex;

// Current queue capacity.
static int queueCapacity;

// Current queue size.
static int queueCount;

// Index of the oldest queue element.
static int queueFrontIndex;

// Index of the next place for a new queue element.
static int queueBackIndex;

// Queue pointer.
static queue_element_t *queue;

// Queue raw buffer size.
static int queueBufferSize;


/* FORWARD DECLARATIONS */

// Adds a new element to the back of the queue.
static void queue_add_element(vkey_t keyCode, bool shiftPressed);

// Removes and returns the front queue element.
// If no element is available, false is returned.
static bool queue_retrieve(vkey_t *keyCode, bool *shiftPressed);


/* FUNCTIONS */

// Keyboard receiving thread function.
// Frequently polls the OS for new key presses and stores them in an internal queue.
static void keyboard_thread(void *args)
{
	// Run until OS terminates this process
	// TODO cleaner exit?
	while(true)
	{
		// Wait for key press message
		msg_type_t newMsgType;
		while((newMsgType = sys_next_message_type()) == MSG_INVALID)
			sys_yield();
		
		// Retrieve message
		msg_key_press_t msg;
		sys_next_message(&msg.header);
		
		// Add key press to queue
		queue_add_element(msg.keyCode, msg.shiftModifier);
	}
}

void keyboard_init()
{
	// Initialize queue variables
	mutex_init(&queueMutex);
	queueCapacity = 0;
	queueCount = 0;
	queueFrontIndex = 0;
	queueBackIndex = 0;
	queue = 0;
	queueBufferSize = 0;
	
	// Run keyboard receiving thread
	run_thread(&keyboard_thread, 0);
}

vkey_t receive_keypress(bool *shiftPressed)
{
	// Wait until key press message arrives
	vkey_t keyCodeTmp;
	bool shiftPressedTmp;
	while(!queue_retrieve(&keyCodeTmp, &shiftPressedTmp))
		sys_yield();
	
	// Shift modifier?
	if(shiftPressed)
		*shiftPressed = shiftPressedTmp;
	
	return keyCodeTmp;
}

bool key_is_printable_character(vkey_t keyCode)
{
	return (VKEY_A <= keyCode && keyCode <= VKEY_Z)
		|| (VKEY_0 <= keyCode && keyCode <= VKEY_9)
		|| (keyCode == VKEY_SPACE) || (keyCode == VKEY_TAB)
		|| (VKEY_SEMICOLON <= keyCode && keyCode <= VKEY_RBRACKET)
		|| (VKEY_KPMULTIPLY <= keyCode && keyCode <= VKEY_KP9);
}

bool key_is_navigation_key(vkey_t keyCode)
{
	return (VKEY_CURSORUP <= keyCode && keyCode <= VKEY_END);
}

char key_to_character(vkey_t keyCode, bool shiftPressed)
{
	// Calculate index for key code conversion table and return resulting character
	int index = 2 * keyCode;
	if(shiftPressed)
		++index;
	return keyCodeToAsciiConversionTable[index];
}

static void queue_add_element(vkey_t keyCode, bool shiftPressed)
{
	// Lock queue
	mutex_acquire(&queueMutex);
	
	// Is there still queue memory available?
	if(queueCount == queueCapacity)
	{
		// Allocate new queue buffer
		int newQueueBufferSize = 2 * queueBufferSize;
		if(newQueueBufferSize == 0)
			newQueueBufferSize = 4096;
		int newQueueCapacity = newQueueBufferSize / sizeof(queue_element_t);
		queue_element_t *newQueue = (queue_element_t *)heap_alloc(newQueueBufferSize);
		
		// Copy remaining elements of old queue
		if(queue)
		{
			int i = queueFrontIndex;
			for(int j = 0; j < queueCount; ++j)
			{
				// Copy entry
				queue_element_t *oldEntry = &queue[i];
				queue_element_t *newEntry = &newQueue[j];
				newEntry->keyCode = oldEntry->keyCode;
				newEntry->shiftPressed = oldEntry->shiftPressed;
				
				// Next index
				++i;
				if(i == queueCount)
					i = 0;
			}
			queueFrontIndex = 0;
			queueBackIndex = queueCount;
		}
		
		// Update queue variables
		queue = newQueue;
		queueCapacity = newQueueCapacity;
		queueBufferSize = newQueueBufferSize;
	}
	
	// Add element to queue
	queue_element_t *element = &queue[queueBackIndex];
	element->keyCode = keyCode;
	element->shiftPressed = shiftPressed;
	
	// One element was added
	++queueCount;
	
	// Update index
	++queueBackIndex;
	if(queueBackIndex == queueCapacity)
		queueBackIndex = 0;
	
	// Unlock queue
	mutex_release(&queueMutex);
}

static bool queue_retrieve(vkey_t *keyCode, bool *shiftPressed)
{
	// Lock queue
	mutex_acquire(&queueMutex);
	
	// Any elements available?
	if(queueCount == 0)
	{
		// Unlock queue
		mutex_release(&queueMutex);
		return false;
	}
	
	// Retrieve front element
	queue_element_t *frontElement = &queue[queueFrontIndex];
	*keyCode = frontElement->keyCode;
	*shiftPressed = frontElement->shiftPressed;
	
	// One element was retrieved
	--queueCount;
	
	// Update front element pointer
	++queueFrontIndex;
	if(queueFrontIndex == queueCapacity)
		queueFrontIndex = 0;
	
	// Unlock queue
	mutex_release(&queueMutex);
	return true;
}