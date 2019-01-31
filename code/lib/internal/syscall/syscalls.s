; Provides implementations for kernel syscalls, that are safely usable from C.

; Creates a system call wrapper routine for functions with 0 to 3 arguments.
; Parameters:
;     - System call function name
;     - System call number
%macro syscallwrapper 2
[global %1]
%1:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters #0, #1, #2 are already in the correct registers
	
	; Do system call
	mov rax, %2
	syscall
	
	; Restore R12
	pop r12
	ret
%endmacro

; Creates a system call wrapper routine for functions with 4 to 6 arguments.
; Parameters:
;     - System call function name
;     - System call number
%macro syscallwrapper4 2
[global %1]
%1:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters #0, #1, #2, #4, #5 are already in the correct registers
	; Parameter #3 must be copied to R10, since RCX is overwritten by SYSCALL
	mov r10, rcx
	
	; Do system call
	mov rax, %2
	syscall
	
	; Restore R12
	pop r12
	ret
%endmacro

syscallwrapper sys_kputs, 0
syscallwrapper sys_exit, 1
syscallwrapper sys_yield, 2
syscallwrapper sys_next_message_type, 3
syscallwrapper sys_next_message, 4
syscallwrapper sys_set_displayed_process, 5
syscallwrapper4 sys_vbe_rectangle, 6
syscallwrapper sys_vbe_render_char, 7
syscallwrapper sys_vbe_get_screen_width, 8
syscallwrapper sys_vbe_get_screen_height, 9
syscallwrapper sys_vbe_set_front_color, 10
syscallwrapper sys_vbe_set_back_color, 11
syscallwrapper sys_vbe_allocate_scroll_buffer, 12
syscallwrapper sys_vbe_set_scroll_position, 13
syscallwrapper sys_vbe_clear, 14
syscallwrapper sys_heap_alloc, 15
syscallwrapper sys_heap_free, 16
syscallwrapper sys_run_thread, 17
syscallwrapper sys_exit_thread, 18
syscallwrapper sys_get_elapsed_milliseconds, 19
syscallwrapper sys_get_network_mac_address, 20
syscallwrapper sys_receive_network_packet, 21
syscallwrapper sys_send_network_packet, 22
syscallwrapper sys_start_process, 23
syscallwrapper sys_info, 24
syscallwrapper sys_set_affinity, 25
syscallwrapper sys_virt_to_phy, 26
syscallwrapper sys_reset, 27
syscallwrapper sys_custom, 28
syscallwrapper4 sys_vbe_draw, 29
syscallwrapper sys_fs_open, 30
syscallwrapper sys_fs_close, 31
syscallwrapper sys_fs_read, 32
syscallwrapper sys_fs_write, 33
syscallwrapper sys_fs_tell, 34
syscallwrapper sys_fs_seek, 35
syscallwrapper sys_fs_create_directory, 36
syscallwrapper sys_fs_list, 37