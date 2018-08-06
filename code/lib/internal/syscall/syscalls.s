; Provides implementations for kernel syscalls, that are safely usable from C.

[global sys_kputs]
sys_kputs:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 0
	syscall
	
	; Restore R12
	pop r12
	ret
	
[global sys_exit]
sys_exit:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 1
	syscall
	
	; Restore R12
	pop r12
	ret
	
[global sys_yield]
sys_yield:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 2
	syscall
	
	; Restore R12
	pop r12
	ret

[global sys_next_message_type]
sys_next_message_type:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 3
	syscall
	
	; Restore R12
	pop r12
	ret

[global sys_next_message]
sys_next_message:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 4
	syscall
	
	; Restore R12
	pop r12
	ret

[global sys_set_displayed_process]
sys_set_displayed_process:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 5
	syscall
	
	; Restore R12
	pop r12
	ret