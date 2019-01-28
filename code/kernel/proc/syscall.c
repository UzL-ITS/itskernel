
#include <proc/syscall.h>
#include <proc/syscalls.h>
#include <cpu/flags.h>
#include <cpu/efer.h>
#include <cpu/msr.h>
#include <cpu/gdt.h>

/*
 * As the kernel is in the higher high we know that the MSB of function
 * pointers is always set to one - therefore we can reuse the bit as a flag
 * which indicates if the syscall should be a direct method call (suitable for
 * anything which doesn't need to perform a context switch) or if it should
 * emulate an interrupt (allowing calls which may switch the context e.g.
 * yield() to be implemented).
 */
#define SYSCALL_DIRECT 0x8000000000000000UL

uintptr_t syscall_table[] =
{
	/*  0 */ (uintptr_t)&sys_trace,
	/*  1 */ (uintptr_t)&sys_exit,
	/*  2 */ (uintptr_t)&sys_yield,
	/*  3 */ (uintptr_t)&sys_next_message_type,
	/*  4 */ (uintptr_t)&sys_next_message,
	/*  5 */ (uintptr_t)&sys_set_displayed_process,
	/*  6 */ (uintptr_t)&sys_vbe_rectangle,
	/*  7 */ (uintptr_t)&sys_vbe_render_char,
	/*  8 */ (uintptr_t)&sys_vbe_get_screen_width,
	/*  9 */ (uintptr_t)&sys_vbe_get_screen_height,
	/* 10 */ (uintptr_t)&sys_vbe_set_front_color,
	/* 11 */ (uintptr_t)&sys_vbe_set_back_color,
	/* 12 */ (uintptr_t)&sys_vbe_allocate_scroll_buffer,
	/* 13 */ (uintptr_t)&sys_vbe_set_scroll_position,
	/* 14 */ (uintptr_t)&sys_vbe_clear,
	/* 15 */ (uintptr_t)&sys_heap_alloc,
	/* 16 */ (uintptr_t)&sys_heap_free,
	/* 17 */ (uintptr_t)&sys_run_thread,
	/* 18 */ (uintptr_t)&sys_exit_thread,
	/* 19 */ (uintptr_t)&sys_get_elapsed_milliseconds,
	/* 20 */ (uintptr_t)&sys_get_network_mac_address,
	/* 21 */ (uintptr_t)&sys_receive_network_packet,
	/* 22 */ (uintptr_t)&sys_send_network_packet,
	/* 23 */ (uintptr_t)&sys_start_process,
	/* 24 */ (uintptr_t)&sys_info,
	/* 25 */ (uintptr_t)&sys_set_affinity,
	/* 26 */ (uintptr_t)&sys_create_directory,
	/* 27 */ (uintptr_t)&sys_create_file,
	/* 28 */ (uintptr_t)&sys_get_file,
	/* 29 */ (uintptr_t)&sys_get_file_info,
	/* 30 */ (uintptr_t)&sys_dump_files,
	/* 31 */ (uintptr_t)&sys_dump_files_get_buffer_size,
	/* 32 */ (uintptr_t)&sys_virt_to_phy,
	/* 33 */ (uintptr_t)&sys_reset,
	/* 34 */ (uintptr_t)&sys_custom,
	/* 35 */ (uintptr_t)&sys_vbe_draw,
};
uint64_t syscall_table_size = sizeof(syscall_table) / sizeof(*syscall_table);

void syscall_init(void)
{
  /* unset SYSCALL_DIRECT bit on syscalls which may perform a context switch */
  syscall_table[1] &= ~SYSCALL_DIRECT;
  syscall_table[2] &= ~SYSCALL_DIRECT;
  syscall_table[18] &= ~SYSCALL_DIRECT;

  /* set the SYSCALL and SYSRET selectors */
  uint64_t star = 0;
  star |= ((uint64_t) (SLTR_KERNEL_DATA | RPL3)) << 48;
  star |=  (uint64_t) SLTR_KERNEL_CODE           << 32;
  msr_write(MSR_STAR, star);

  /*
   * how the selectors are determined:
   *
   * SYSRET:
   *   CS = sysret_sel + 16
   *   SS = sysret_sel + 8
   *
   * SYSCALL:
   *   CS = syscall_sel
   *   SS = syscall_sel + 8
   *
   * Required GDT layout:
   *
   *    0: null
   *    8: kernel code (syscall_sel)
   *   16: kernel data (sysret_sel)
   *   24: user data
   *   32: user code
   *   (additional entries e.g. TSS at the end)
   */

  /* set the long mode SYSCALL target RIP */
  msr_write(MSR_LSTAR, (uint64_t) &syscall_stub);

  /*
   * set the flags to clear upon a SYSCALL:
   *
   *   - DF is cleared, the AMD64 System V ABI states this must be done before
   *     a function call
   *
   *   - IF is cleared, this is to avoid a race condition where an interrupt
   *     happens in supervisor mode before RSP and GS have been set (if an
   *     interrupt did happen, RSP0 would not be loaded into RSP from the TSS
   *     and the interrupt would run in the user stack, and the GS stuff would
   *     get messy)
   */
  msr_write(MSR_SFMASK, FLAGS_DF | FLAGS_IF);

  /* enable the SCE flag in the EFER register */
  efer_write(efer_read() | EFER_SCE);
}
