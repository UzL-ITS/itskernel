
#include <vbe/vbe.h>
#include <mm/phy32.h>

// ARGB foreground and background colors (32-bit color).
static uint32_t colorFront = 0x00FFFFFF;
static uint32_t colorBack = 0x00000000;

// Render buffer.
static uint16_t renderBufferWidth;
static uint16_t renderBufferHeight;
static uint32_t *renderBuffer;

/*static const char *itoa(uint16_t val)
{
	char numbers[10] = {0};
	int index = 9;
	while(val > 0 && index >= 0)
	{
		uint16_t lastDigit = val % 10;
		numbers[index] = (char)('0' + lastDigit);
		val = val / 10;
		--index;
	}
	return numbers;
}*/

int vbe_rectangle(uint16_t posX, uint16_t posY, uint16_t width, uint16_t height)
{
	// Sanity checks for parameters
	if(posX >= renderBufferWidth || posX + width > renderBufferWidth)
		return -1;
	if(posY >= renderBufferHeight || posY + height > renderBufferHeight)
		return -2;
	
	// Draw rectangle
	uint16_t xMax = posX + width;
	uint16_t yMax = posY + height;
	for(uint16_t y = posY; y <= yMax; ++y)
	{
		uint32_t *pixel = renderBuffer + y * renderBufferWidth + posX;
		for(uint16_t x = posX; x <= xMax; ++x)
		{
			*pixel = colorFront;
			++pixel;
		}
	}
	return 0;
}

int vbe_render_char(uint16_t posX, uint16_t posY, char c)
{
	// Sanity checks for parameters
	if(posX >= renderBufferWidth)
		return -1;
	if(posY >= renderBufferHeight)
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
		uint32_t *renderBufferPtr = renderBuffer + (posY + i) * renderBufferWidth + posX;
		for(int j = 0; j < 8; ++j)
			if((bitmap & (0x80 >> j)) != 0x00)
				renderBufferPtr[j] = colorFront;
			else
				renderBufferPtr[j] = colorBack;
	}
	
	return 0;
}

int vbe_render_string(uint16_t posX, uint16_t posY, char *str, int strLength)
{
	// Render individual chars
	for(int i = 0; i < strLength; ++i)
	{
		int status = vbe_render_char(posX + i * VBE_FONT_CHARACTER_WIDTH, posY, str[i]);
		if(status != 0)
			return status;
	}
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

uint16_t vbe_get_screen_width()
{
	return renderBufferWidth;
}

uint16_t vbe_get_screen_height()
{
	return renderBufferHeight;
}

void vbe_set_front_color(uint8_t r, uint8_t g, uint8_t b)
{
	colorFront = ((uint32_t)r << 16) | ((uint16_t)g << 8) | b;
}

void vbe_set_back_color(uint8_t r, uint8_t g, uint8_t b)
{
	colorBack = ((uint32_t)r << 16) | ((uint16_t)g << 8) | b;
}