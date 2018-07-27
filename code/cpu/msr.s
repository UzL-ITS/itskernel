[global msr_read]
msr_read:
  push rbp
  mov rbp, rsp
  mov rcx, rdi
  xor rax, rax
  xor rdx, rdx
  rdmsr
  shl rdx, 32
  or rax, rdx
  pop rbp
  ret

[global msr_write]
msr_write:
  push rbp
  mov rbp, rsp
  mov rcx, rdi
  mov rdx, rsi
  shr rdx, 32
  mov rax, rsi
  wrmsr
  pop rbp
  ret
