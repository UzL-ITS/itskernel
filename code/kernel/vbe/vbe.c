
#include <trace/trace.h>
#include <vbe/vbe.h>
#include <mm/phy32.h>
#include <mm/heap.h>
#include <panic/panic.h>
#include <lock/spinlock.h>


// VBE drawing context descriptor.
typedef struct
{
	// Usage flag.
	bool inUse;
	
	// Back buffers.
	uint32_t *previousBuffer; // Holds the previous image (copy of what is rendered on the screen)
	uint32_t *currentBuffer; // Holds the current image, scrollable
	
	// Height of the scrollable buffer.
	uint32_t currentBufferHeight;
	
	// Scroll position.
	uint32_t scrollY;
	
	// Colors.
	uint32_t colorFront;
	uint32_t colorBack;
} vbe_context_t;


// ARGB foreground and background colors (32-bit color).
static uint32_t colorFront;
static uint32_t colorBack;

// Render buffer.
static uint32_t renderBufferWidth;
static uint32_t renderBufferHeight;
static uint32_t *renderBuffer;

// Drawing contexts containing background buffers with the same dimensions as the render buffer,
// which are then blitted to video memory (instead of directly writing to the render buffer).
// TODO use linked list here eventually
#define VBE_CONTEXT_COUNT 12
static vbe_context_t contexts[VBE_CONTEXT_COUNT];
static int currentContext = 0;

// Back buffers of the current drawing context.
static uint32_t *previousBuffer; // Holds the previously rendered image
static uint32_t *currentBuffer; // Holds the current image

// Height of the scrollable buffer of the current drawing context.
static uint32_t currentBufferHeight;

// Scroll position of the scrollable buffer of the current drawing context.
static uint32_t scrollY;

// Determines whether context #0 buffers actually point to virtual memory.
// This is needed since debug outputs might be printed before the memory manager is up.
static bool buffersAllocated = false;

// Lock to ensure ordered access to the context management.
static spinlock_t vbeContextLock = SPIN_UNLOCKED;


// Blits the given rectangle of the current buffer to video memory.
// The coordinates are passed as absolute values (these are then translated by the scroll offset).
static void _vbe_blit_to_video_memory(int contextId, uint32_t posX, uint32_t posY, uint32_t width, uint32_t height)
{	
	// Back buffers initialized yet?
	if(!buffersAllocated)
		return;

	// Avoid race conditions
	spin_lock(&vbeContextLock);
	
	// Only draw current context
	if(contextId != currentContext)
	{
		spin_unlock(&vbeContextLock);
		return;
	}
	
	// Check parameters
	if(posY < scrollY || posY >= scrollY + renderBufferHeight)
	{
		spin_unlock(&vbeContextLock);
		return;
	}
	
	// Calculate render buffer pixels indices
	uint32_t renderBufferX = posX;
	uint32_t renderBufferY = posY - scrollY;
	uint32_t renderBufferXMax = renderBufferX + width;
	uint32_t renderBufferYMax = renderBufferY + height;
	if(renderBufferYMax > renderBufferHeight)
		renderBufferYMax = renderBufferHeight;
	
	// Run through rectangle pixels
	uint32_t xSteps = renderBufferXMax - renderBufferX;
	uint32_t ySteps = renderBufferYMax - renderBufferY;
	for(uint32_t y = 0; y < ySteps; ++y)
	{
		uint32_t *pixelRenderBuffer = renderBuffer + (renderBufferY + y) * renderBufferWidth + renderBufferX;
		uint32_t *pixelCurrent = currentBuffer + (posY + y) * renderBufferWidth + renderBufferX;
		uint32_t *pixelPrevious = previousBuffer + (renderBufferY + y) * renderBufferWidth + renderBufferX;
		for(uint32_t x = 0; x < xSteps; ++x)
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
	
	spin_unlock(&vbeContextLock);
}

int vbe_create_context()
{
	spin_lock(&vbeContextLock);
	
	// Try to find unused context
	for(int i = 0; i < VBE_CONTEXT_COUNT; ++i)
		if(!contexts[i].inUse)
		{
			// Get drawing context
			vbe_context_t *context = &contexts[i];
			context->inUse = true;
			spin_unlock(&vbeContextLock);
	
			// Initialize context
			context->previousBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferHeight, VM_R | VM_W);
			context->currentBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferHeight, VM_R | VM_W);
			context->currentBufferHeight = renderBufferHeight;
			context->scrollY = 0;
			context->colorFront = 0x00FFFFFF;
			context->colorBack = 0x00000000;
			if(!context->previousBuffer || !context->currentBuffer)
			{
				panic("Error creating VBE context\n");
				return -2; // TODO better error codes
			}
			
			// Done, return context identifier
			trace_printf("Created VBE context #%d\n", i);
			return i;
		}
		
	spin_unlock(&vbeContextLock);
	return -1;
}

void vbe_show_context(int contextId)
{
	// Check whether context ID is valid
	if(contextId < 0 || contextId >= VBE_CONTEXT_COUNT || !contexts[contextId].inUse)
		return;
	
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	spin_lock(&vbeContextLock);
	{
		// Load context data
		previousBuffer = context->previousBuffer;
		currentBuffer = context->currentBuffer;
		currentBufferHeight = context->currentBufferHeight;
		scrollY = context->scrollY;
		colorFront = context->colorFront;
		colorBack = context->colorBack;
		currentContext = contextId;
	
		if(buffersAllocated)
		{
			// Update "previous" buffer (it still contains the pixels when the drawing context was rendered last time), and redraw whole screen
			uint32_t *currentBufferScrolledPtr = currentBuffer + scrollY * renderBufferWidth;
			for(uint32_t i = 0; i < renderBufferWidth * renderBufferHeight; ++i)
			{
				uint32_t pixel = currentBufferScrolledPtr[i];
				previousBuffer[i] = pixel;
				renderBuffer[i] = pixel;
			}
		}
	
	}
	spin_unlock(&vbeContextLock);
}

int vbe_rectangle(int contextId, uint32_t posX, uint32_t posY, uint32_t width, uint32_t height)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Sanity checks for parameters
	if(posX >= renderBufferWidth || posX + width > renderBufferWidth)
		return -1;
	if(posY >= context->currentBufferHeight || posY + height > context->currentBufferHeight)
		return -2;
	
	// Draw rectangle
	uint32_t xMax = posX + width;
	uint32_t yMax = posY + height;
	for(uint32_t y = posY; y < yMax; ++y)
	{
		uint32_t *pixel = context->currentBuffer + y * renderBufferWidth + posX;
		for(uint32_t x = posX; x < xMax; ++x)
		{
			*pixel = context->colorFront;
			++pixel;
		}
	}
	_vbe_blit_to_video_memory(contextId, posX, posY, width, height);
	return 0;
}

int vbe_render_char(int contextId, uint32_t posX, uint32_t posY, char c)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Sanity checks for parameters
	if(posX >= renderBufferWidth)
		return -1;
	if(posY >= context->currentBufferHeight)
		return -2;
	if(c < ' ' || c > '~')
		return -3;
	
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
		context->currentBufferHeight = renderBufferHeight;
		context->scrollY = 0;
		context->colorFront = 0x00FFFFFF;
		context->colorBack = 0x00000000;
		vbe_show_context(VBE_KERNEL_CONTEXT);
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
	contexts[VBE_KERNEL_CONTEXT].previousBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferHeight, VM_R | VM_W);
	contexts[VBE_KERNEL_CONTEXT].currentBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * renderBufferHeight, VM_R | VM_W);
	if(!contexts[VBE_KERNEL_CONTEXT].previousBuffer || !contexts[VBE_KERNEL_CONTEXT].currentBuffer)
		panic("Could not reserve %d bytes of memory for VBE back buffer, heap allocator broken?", 4 * renderBufferWidth * renderBufferHeight);
	
	// Copy whole current video memory buffer to back buffers (so we don't lose previous debug outputs)
	for(uint32_t i = 0; i < renderBufferWidth * renderBufferHeight; ++i)
	{
		uint32_t currentPixel = renderBuffer[i];
		contexts[VBE_KERNEL_CONTEXT].previousBuffer[i] = currentPixel;
		contexts[VBE_KERNEL_CONTEXT].currentBuffer[i] = currentPixel;
	}
	buffersAllocated = true;
	
	// Reload context to update local variables
	vbe_show_context(VBE_KERNEL_CONTEXT);
}

uint32_t vbe_get_screen_width()
{
	return renderBufferWidth;
}

uint32_t vbe_get_screen_height()
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
	spin_lock(&vbeContextLock);
	{
		if(contextId == currentContext)
			colorFront = context->colorFront;
	}
	spin_unlock(&vbeContextLock);
}

void vbe_set_back_color(int contextId, uint8_t r, uint8_t g, uint8_t b)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Update color in context
	context->colorBack = ((uint32_t)r << 16) | ((uint16_t)g << 8) | b;
	
	// Current context? => Update local color too
	spin_lock(&vbeContextLock);
	{
		if(contextId == currentContext)
			colorBack = context->colorBack;
	}
	spin_unlock(&vbeContextLock);
}

bool vbe_allocate_scroll_buffer(int contextId, uint32_t height)
{
	// The kernel context is fixed
	if(contextId == VBE_KERNEL_CONTEXT)
		return false;
	
	spin_lock(&vbeContextLock);
	
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Try to allocate new buffer first
	uint32_t *newBuffer = (uint32_t *)heap_alloc(4 * renderBufferWidth * height, VM_R | VM_W);
	if(!newBuffer)
	{
		// Probably not enough memory available
		spin_unlock(&vbeContextLock);
		return false;
	}
	
	// Free old buffer and assign new one
	heap_free(context->currentBuffer);
	context->currentBuffer = newBuffer;
	context->currentBufferHeight = height;
	context->scrollY = 0;
	
	// If this is the current context, update it too
	if(currentContext == contextId)
	{
		currentBuffer = newBuffer;
		currentBufferHeight = height;
		scrollY = 0;
	}
	
	spin_unlock(&vbeContextLock);
	return true;
}

void vbe_set_scroll_position(int contextId, uint32_t y)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Check parameters
	if(y >= context->currentBufferHeight - renderBufferHeight)
		y = context->currentBufferHeight - renderBufferHeight - 1;
	
	// Update position
	context->scrollY = y;
	
	// If current context, update position there too
	spin_lock(&vbeContextLock);
	{
		if(contextId == currentContext)
			scrollY = y;
	}
	spin_unlock(&vbeContextLock);

	// Redraw, if current context
	_vbe_blit_to_video_memory(contextId, 0, y, renderBufferWidth, renderBufferHeight);
}

void vbe_clear(int contextId)
{
	// Get drawing context
	vbe_context_t *context = &contexts[contextId];
	
	// Clear whole buffer
	for(uint32_t i = 0; i < renderBufferWidth * context->currentBufferHeight; ++i)
		context->currentBuffer[i] = context->colorBack;
	
	// Redraw, if current context
	_vbe_blit_to_video_memory(contextId, 0, context->scrollY, renderBufferWidth, renderBufferHeight);
}