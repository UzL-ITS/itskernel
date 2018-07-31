
#include <trace/trace.h>
#include <vbe/vbe.h>
#include <mm/phy32.h>
#include <mm/heap.h>
#include <stdbool.h>
#include <panic/panic.h>


// VBE drawing context descriptor.
typedef struct
{
	// Usage flag.
	bool inUse;
	
	// Back buffers.
	uint32_t *previousBuffer; // Holds the previous image
	uint32_t *currentBuffer; // Holds the current image
	
	// Colors.
	uint32_t colorFront;
	uint32_t colorBack;
} vbe_context_t;

// ARGB foreground and background colors (32-bit color).
static uint32_t colorFront;
static uint32_t colorBack;

// Render buffer.
static uint16_t renderBufferWidth;
static uint16_t renderBufferHeight;
static uint32_t *renderBuffer;

// Drawing contexts containing background buffers with the same dimensions as the render buffer,
// which are then blitted to video memory (instead of directly writing to the render buffer).
#define VBE_CONTEXT_COUNT 10
static vbe_context_t contexts[VBE_CONTEXT_COUNT];
int currentContext = 0;

// Back buffers of the current drawing context.
static uint32_t *previousBuffer; // Holds the previously rendered image
static uint32_t *currentBuffer; // Holds the current image

// Determines whether context #0 buffers actually point to virtual memory.
// This is needed since debug outputs might be printed before the memory manager is up.
static bool buffersAllocated = false;


// Blits the given rectangle to video memory.
static void _vbe_blit_to_video_memory(int contextId, uint16_t posX, uint16_t posY, uint16_t width, uint16_t height)
{	
	// Back buffers initialized yet?
	if(!buffersAllocated)
		return;

	// Only draw current context
	if(contextId != currentContext)
		return;
	
	// Run through rectangle pixels
	uint16_t xMax = posX + width;
	uint16_t yMax = posY + height;
	for(uint16_t y = posY; y <= yMax; ++y)
	{
		uint32_t *pixelRenderBuffer = renderBuffer + y * renderBufferWidth + posX;
		uint32_t *pixelCurrent = currentBuffer + y * renderBufferWidth + posX;
		uint32_t *pixelPrevious = previousBuffer + y * renderBufferWidth + posX;
		for(uint16_t x = posX; x <= xMax; ++x)
		{
			// Transfer pixel only when it was changed
			uint32_t pixelCurrentVal = *pixelCurrent;
			if(*pixelPrevious != pixelCurrentVal)
			{
				*pixelPrevious = pixelCurrentVal;
				*pixelRenderBuffer = pixelCurrentVal;
			}
			++pixelRenderBuffer;
			++pixelCurrent;
			++pixelPrevious;
		}
	}
}

void vbe_redraw()
{
	// Copy back buffer into video memory
	for(int i = 0; i < renderBufferWidth * renderBufferHeight; ++i)
		renderBuffer[i] = previousBuffer[i];
}

int vbe_create_context()
{
	// Try to find unused context
	for(int i = 0; i < VBE_CONTEXT_COUNT; ++i)
		if(!contexts[i].inUse)
		{
			// Get drawing context
			vbe_context_t *context = &contexts[i];
	
			// Initialize context
			context->previousBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferWidth, VM_R | VM_W);
			context->currentBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferWidth, VM_R | VM_W);
			context->colorFront = 0x00FFFFFF;
			context->colorBack = 0x00000000;
			if(!context->previousBuffer || !context->currentBuffer)
				return -2; // TODO better error codes
			
			// Context is up, return its identifier
			context->inUse = true;
			return i;
		}
	return -1;
}

void vbe_show_context(int contextId)
{
	// Check whether context ID is valid
	if(contextId < 0 || contextId >= VBE_CONTEXT_COUNT || !contexts[contextId].inUse)
		return;
	
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Load context data
	previousBuffer = context->previousBuffer;
	currentBuffer = context->currentBuffer;
	colorFront = context->colorFront;
	colorBack = context->colorBack;
	currentContext = contextId;
	
	if(buffersAllocated)
	{
		// Update "previous" buffer (it still contains the pixels when the drawing context was rendered last time)
		for(int i = 0; i < renderBufferWidth * renderBufferHeight; ++i)
			previousBuffer[i] = currentBuffer[i];
	
		// Redraw
		vbe_redraw();
	}
}

int vbe_rectangle(int contextId, uint16_t posX, uint16_t posY, uint16_t width, uint16_t height)
{
	// Sanity checks for parameters
	if(posX >= renderBufferWidth || posX + width > renderBufferWidth)
		return -1;
	if(posY >= renderBufferHeight || posY + height > renderBufferHeight)
		return -2;
	
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Draw rectangle
	uint16_t xMax = posX + width;
	uint16_t yMax = posY + height;
	for(uint16_t y = posY; y <= yMax; ++y)
	{
		uint32_t *pixel = context->currentBuffer + y * renderBufferWidth + posX;
		for(uint16_t x = posX; x <= xMax; ++x)
		{
			*pixel = context->colorFront;
			++pixel;
		}
	}
	_vbe_blit_to_video_memory(contextId, posX, posY, width, height);
	return 0;
}

int vbe_render_char(int contextId, uint16_t posX, uint16_t posY, char c)
{
	// Sanity checks for parameters
	if(posX >= renderBufferWidth)
		return -1;
	if(posY >= renderBufferHeight)
		return -2;
	if(c < ' ' || c > '~')
		return -3;
	
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Get char array index
	int charIndex = c - ' ';
	
	// Run through character lines...
	for(int i = 0; i < VBE_FONT_CHARACTER_HEIGHT; ++i)
	{
		// ...and render bitmap of each line
		uint8_t bitmap = font_data[charIndex][i];
		uint32_t *renderBufferPtr = context->currentBuffer + (posY + i) * renderBufferWidth + posX;
		for(int j = 0; j < 8; ++j)
			if((bitmap & (0x80 >> j)) != 0x00)
				renderBufferPtr[j] = context->colorFront;
			else
				renderBufferPtr[j] = context->colorBack;
	}
	_vbe_blit_to_video_memory(contextId, posX, posY, VBE_FONT_CHARACTER_WIDTH, VBE_FONT_CHARACTER_HEIGHT);
	return 0;
}

void vbe_init(multiboot_t *multiboot)
{
	// Get VBE tag, if it exists (GRUB should setup VBE for us)
	multiboot_tag_t *vbeTag = multiboot_get(multiboot, MULTIBOOT_TAG_VBE);
	if(vbeTag)
	{
		// Get render buffer
		renderBuffer = (uint32_t *)aphy32_to_virt(vbeTag->vbe.vbe_mode_info.physbase);
		renderBufferWidth = vbeTag->vbe.vbe_mode_info.Xres;
		renderBufferHeight = vbeTag->vbe.vbe_mode_info.Yres;
		
		// Clear drawing context array
		for(int i = 0; i < VBE_CONTEXT_COUNT; ++i)
			contexts[i].inUse = false;
		
		// Initialize kernel UI context
		// At the beginning all buffers are the same (no virtual memory manager yet)
		vbe_context_t *context = &contexts[VBE_KERNEL_CONTEXT];
		context->inUse = true;
		context->previousBuffer = renderBuffer;
		context->currentBuffer = renderBuffer;
		context->colorFront = 0x00FFFFFF;
		context->colorBack = 0x00000000;
		vbe_show_context(VBE_KERNEL_CONTEXT);
		
		// TEST
		//vbe_rectangle(50, 100, 50, 100);
		//vbe_render_string(200, 200, "This is a $5 test string?! a(x) = b[y]_3 + \"test\"...", 62);
	}
	else
	{
		// Render error message into default framebuffer
		uint8_t *framebuffer = (uint8_t *)aphy32_to_virt(multiboot_get(multiboot, MULTIBOOT_TAG_FRAMEBUF)->framebuffer.framebuffer_addr);
		char *str = "Error: No VBE tag";
		for(int i = 0; i < 17; ++i)
		{
			framebuffer[2 * i + 1] = 0x0F;
			framebuffer[2 * i] = str[i];
		}
	}
}

void vbe_init_back_buffers()
{
	// Get memory for both buffers
	contexts[VBE_KERNEL_CONTEXT].previousBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferWidth, VM_R | VM_W);
	contexts[VBE_KERNEL_CONTEXT].currentBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferWidth, VM_R | VM_W);
	if(!contexts[VBE_KERNEL_CONTEXT].previousBuffer || !contexts[VBE_KERNEL_CONTEXT].currentBuffer)
		panic("Could not reserve %d bytes of memory for VBE back buffer, heap allocator broken?", 4 * renderBufferWidth * renderBufferWidth);
	
	// Copy whole current video memory buffer to back buffers (so we don't lose previous debug outputs)
	for(int i = 0; i < renderBufferWidth * renderBufferHeight; ++i)
	{
		uint32_t currentPixel = renderBuffer[i];
		contexts[VBE_KERNEL_CONTEXT].previousBuffer[i] = currentPixel;
		contexts[VBE_KERNEL_CONTEXT].currentBuffer[i] = currentPixel;
	}
	buffersAllocated = true;
	
	// Reload context to update local variables
	vbe_show_context(VBE_KERNEL_CONTEXT);
}

uint16_t vbe_get_screen_width()
{
	return renderBufferWidth;
}

uint16_t vbe_get_screen_height()
{
	return renderBufferHeight;
}

void vbe_set_front_color(int contextId, uint8_t r, uint8_t g, uint8_t b)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Update color in context
	context->colorFront = ((uint32_t)r << 16) | ((uint16_t)g << 8) | b;
	
	// Current context? => Update local color too
	if(contextId == currentContext)
		colorFront = context->colorFront;
}

void vbe_set_back_color(int contextId, uint8_t r, uint8_t g, uint8_t b)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Update color in context
	context->colorBack = ((uint32_t)r << 16) | ((uint16_t)g << 8) | b;
	
	// Current context? => Update local color too
	if(contextId == currentContext)
		colorBack = context->colorBack;
}