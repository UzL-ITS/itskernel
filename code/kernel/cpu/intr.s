[global intr_enable]
intr_enable:
  push rbp
  mov rbp, rsp
  sti
  pop rbp
  ret

[global intr_disable]
intr_disable:
  push rbp
  mov rbp, rsp
  cli
  pop rbp
  ret
