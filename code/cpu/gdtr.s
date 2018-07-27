[global gdtr_install]
gdtr_install:
  push rbp
  mov rbp, rsp
  lgdt [rdi]
  mov ax, dx
  mov ss, ax
  mov ax, 0
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov rax, qword .trampoline
  
  ; Push CS value to jump into new code segment
  push rsi
  push rax
  o64 retf ; Return far (jump into different code segment)
.trampoline:
  pop rbp
  ret
