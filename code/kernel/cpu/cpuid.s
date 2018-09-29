[global cpu_id]
cpu_id:
	push rbp
	mov rbp, rsp

	mov r10, rdx
	mov r11, rcx
	mov rax, rdi
	mov rdi, rbx

	cpuid

	mov dword [rsi], eax
	mov dword [r10], ebx
	mov dword [r11], ecx
	mov dword [r8], edx
	mov rbx, rdi

	pop rbp
	ret
  
[global cpu_id_special]
cpu_id_special:
	push rbx

	mov r10, rdx
	mov r11, rcx

	mov rax, rdi
	mov rcx, rsi

	cpuid

	mov dword [r10], eax
	mov dword [r11], ebx
	mov dword [r8], ecx
	mov dword [r9], edx

	pop rbx
	ret
