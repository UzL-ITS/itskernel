; Provides implementations for kernel syscalls, that are safely usable from C.

[global testprintf]
testprintf:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 0
	syscall
	
	; Restore R12
	pop r12
	ret

[global next_message_type]
next_message_type:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 3
	syscall
	
	; Restore R12
	pop r12
	ret

[global next_message]
next_message:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 4
	syscall
	
	; Restore R12
	pop r12
	ret

[global set_displayed_process]
set_displayed_process:
	; Preserve R12 (is trashed by syscall handler)
	push r12

	; Parameters are already in the correct registers
	
	; Do system call
	mov rax, 5
	syscall
	
	; Restore R12
	pop r12
	ret