[global tlb_invlpg]
tlb_invlpg:
  push rbp
  mov rbp, rsp
  invlpg [rdi]
  pop rbp
  ret

[global tlb_flush]
tlb_flush:
  push rbp
  mov rbp, rsp
  mov rax, cr3
  mov cr3, rax
  pop rbp
  ret
