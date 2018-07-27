[global halt_once]
halt_once:
  push rbp
  mov rbp, rsp
  hlt
  pop rbp
  ret

[global halt_forever]
halt_forever:
  push rbp
  mov rbp, rsp
  .hang:
    hlt
    jmp .hang
