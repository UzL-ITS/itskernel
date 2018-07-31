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
