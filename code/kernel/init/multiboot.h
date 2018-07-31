
#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#include <stdint.h>

/* the multiboot magic number */
#define MULTIBOOT_MAGIC 0x36D76289

/* the multiboot info tag numbers */
#define MULTIBOOT_TAG_TERMINATOR 0
#define MULTIBOOT_TAG_CMDLINE    1
#define MULTIBOOT_TAG_BOOT_LDR   2
#define MULTIBOOT_TAG_MODULE     3
#define MULTIBOOT_TAG_MEM        4
#define MULTIBOOT_TAG_BOOT_DEV   5
#define MULTIBOOT_TAG_MMAP       6
#define MULTIBOOT_TAG_VBE        7
#define MULTIBOOT_TAG_FRAMEBUF   8
#define MULTIBOOT_TAG_ELF        9
#define MULTIBOOT_TAG_APM        10

/* indicates the type of a memory region */
#define MULTIBOOT_MMAP_AVAILABLE    1
#define MULTIBOOT_MMAP_RESERVED     2
#define MULTIBOOT_MMAP_ACPI_RECLAIM 3
#define MULTIBOOT_MMAP_ACPI_NVS     4
#define MULTIBOOT_MMAP_BAD          5

/* align the given address up to the next 8 byte boundary */
#define MULTIBOOT_ALIGN(x) (((x) + 7) & 0xFFFFFFFFFFFFFFF8)

/* multiboot information structure */
typedef struct
{
  uint32_t total_size;
  uint32_t reserved;
} __attribute__((__packed__)) multiboot_t;

typedef struct
{
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t reserved;
} __attribute__((__packed__)) multiboot_mmap_entry_t;

/* multiboot tag structure */
typedef struct
{
  /* general information */
  uint32_t type;
  uint32_t size;

  /* type-specific information */
  union
  {
    /* mmap tag */
    struct
    {
      uint32_t entry_size;
      uint32_t entry_version;
      /* entries follow here */
    } __attribute__((__packed__)) mmap;

    /* module tag */
    struct
    {
      uint32_t mod_start;
      uint32_t mod_end;
      char string[1];
    } __attribute__((__packed__)) module;

    /* cmdline tag */
    struct
    {
      char string[1];
    } __attribute__((__packed__)) cmdline;

    /* elf tag */
    struct
    {
      uint32_t sh_num;
      uint32_t sh_entsize;
      uint32_t sh_shstrndx;
      char data[1];
    } __attribute__((__packed__)) elf;
	
	// framebuffer tag
	struct
	{
		uint64_t framebuffer_addr;
		uint32_t framebuffer_pitch;
		uint32_t framebuffer_width;
		uint32_t framebuffer_height;
		uint8_t framebuffer_bpp;
		uint8_t framebuffer_type;
		uint8_t reserved;
		// color_info follows
	} __attribute__((__packed__)) framebuffer;
	
	// VBE tag
	struct
	{
		uint16_t vbe_mode;
		uint16_t vbe_interface_seg;
		uint16_t vbe_interface_off;
		uint16_t vbe_interface_len;
		uint8_t external_specification1[512];
		
		struct
		{
			uint16_t attributes;
			uint8_t  winA, winB;
			uint16_t granularity;
			uint16_t winsize;
			uint16_t segmentA, segmentB;
			uint32_t realFctPtr;
			uint16_t pitch;

			uint16_t Xres, Yres;
			uint8_t  Wchar, Ychar, planes, bpp, banks;
			uint8_t  memory_model, bank_size, image_pages;
			uint8_t  reserved0;

			uint8_t  red_mask, red_position;
			uint8_t  green_mask, green_position;
			uint8_t  blue_mask, blue_position;
			uint8_t  rsv_mask, rsv_position;
			uint8_t  directcolor_attributes;

			uint32_t physbase;
			uint32_t reserved1;
			uint16_t reserved2;
			// VBE 3.0 part follows, see vbe3.pdf in references directory
		} __attribute__ ((packed)) vbe_mode_info;
	} __attribute__((__packed__)) vbe;
  };
} __attribute__((__packed__)) multiboot_tag_t;

multiboot_tag_t *multiboot_get(multiboot_t *multiboot, uint32_t type);
multiboot_tag_t *multiboot_get_after(multiboot_t *multiboot, multiboot_tag_t *start, uint32_t type);

#endif
