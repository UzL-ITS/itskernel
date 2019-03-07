; CPUID functions.

; Runs the CPUID instruction with the given input and output registers.
; Parameters:
;     - rdi: EAX in
;     - rsi: Pointer to EAX out
;     - rdx: Pointer to EBX out
;     - rcx: Pointer to ECX out
;     - r8: Pointer to EDX out
[global cpuid]
cpuid:
	; Save pointers, since registers will be overwritten
	mov r10, rdx
	mov r11, rcx
	
	; Set input parameter
	mov rax, rdi
	
	; Save non-volatile register
	mov rdi, rbx

	; CPUID...
	cpuid
	
	; Store output values
	mov dword [rsi], eax
	mov dword [r10], ebx
	mov dword [r11], ecx
	mov dword [r8], edx
	
	; Restore non-volatile register
	mov rbx, rdi
	
	; Done
	ret
 
; Runs the CPUID instruction with the given input and output registers.
; Parameters:
;     - rdi: EAX in
;     - rsi: ECX in
;     - rdx: Pointer to EAX out
;     - rcx: Pointer to EBX out
;     - r8: Pointer to ECX out
;     - r9: Pointer to EDX out
[global cpuid_special]
cpuid_special:
	; Save non-volatile register
	push rbx
	
	; Save pointers, since registers will be overwritten
	mov r10, rdx
	mov r11, rcx

	; Set input parameters
	mov rax, rdi
	mov rcx, rsi

	; CPUID...
	cpuid
	
	; Store output values
	mov dword [r10], eax
	mov dword [r11], ebx
	mov dword [r8], ecx
	mov dword [r9], edx

	; Done
	pop rbx
	ret
