[bits 64]

; Parameters: rdi -> addr
;             rsi -> count
[global victimcode]
victimcode:
	xor rcx, rcx
	
	victimcode_loop:
		lfence
		rdtsc
		mov r8, rax
		mov r9, rdx
		
		mov rax, [rdi]
		
		lfence
		rdtsc
		
		shl rdx, 32
		or rax, rdx
		
		shl r9, 32
		or r8, r9
		
		sub rax, r8
		add rcx, rax
		
		dec rsi
		jne victimcode_loop
	
	mov rax, rcx
	ret

; Parameters: rdi -> addr
[global attackercode]
attackercode:
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	mov [rdi], rax
	jmp attackercode





; Parameters: rdi -> evictionBuffer
[global measure]
measure:
	
	lfence
	rdtsc
	mov r8, rax
	mov r9, rdx
	
	mov rax, [rdi]
	
	lfence
	rdtsc
	
	shl rdx, 32
	or rax, rdx
	
	shl r9, 32
	or r8, r9
	
	sub rax, r8
	
	ret