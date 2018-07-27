[global flags_read]
flags_read:
  push rbp
  mov rbp, rsp
  pushf
  pop rax
  pop rbp
  ret

[global flags_write]
flags_write:
  push rbp
  mov rbp, rsp
  push rdi
  popf
  pop rbp
  ret
