; Provides implementations for kernel syscalls that are callable from C.

[global testprintf]
testprintf:
	; Parameter is already in RDI
	
	; Do system call
	mov rax, 0
	syscall
	ret