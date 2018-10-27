[extern syscall_table]
[extern syscall_table_size]

; see syscall.c for explanation of this flag
SYSCALL_FAST equ 0x8000000000000000

[global syscall_stub]
syscall_stub:
  ; switch the GS base to the kernel's
  swapgs

  ; switch to the kernel stack (use R12, a callee-saved register - so care must
  ; be taken in user-space - as a temporary)
  mov r12, [gs:16]   ; find current thread_t
  mov [r12 + 8], rsp ; save current RSP in thread->syscall_rsp
  mov rsp, [r12]     ; load new RSP from thread->kernel_rsp

  ; it is safe for to re-enable interrupts now, for information about the
  ; race condition see syscall.c where the SYSCALL flags mask is set
  ; TODO check if scheduler interrupts during syscalls can break things
  ;      currently we just increase the intr_mask_count such that interrupts aren't accidentally re-enabled during syscall handling
  ;sti

  ; preserve RCX and R11, these are used by SYSCALL/SYSRET
  push rcx
  push r11

  ; preserve RBP, we set it to zero to terminate stack traces in kernel land
  ; TODO consider making user land preserve it?
  push rbp

  ; check if the syscall number is out of range
  mov r11, qword syscall_table_size
  cmp rax, [r11]
  jge .invalid_syscall

  ; find address of syscall function
  mov r11, qword syscall_table
  shl rax, 3
  add r11, rax
  mov r11, [r11]

  ; jump to the faux interrupt code if the SYSCALL is not marked as fast
  mov rax, SYSCALL_FAST
  test r11, rax
  jz .faux_intr
  
  ; Decrease mask count such that interrupts aren't accidentally re-enabled
  ; Faux interrupts increase the mask count themselves!
  inc qword [gs:8]

  ; call the syscall function in the kernel directly
  mov rcx, r10 ; syscall ABI uses R10 instead of RCX, fix that for normal ABI
  mov rbp, 0   ; terminate stack traces here
  call r11
  
  ; Restore mask count
  ; Faux interrupts decrease the mask count themselves!
  dec qword [gs:8]

.invalid_syscall:
  ; TODO: we probably want some sort of error upon an invalid syscall
  ; i.e. mov rax, <error code>

.post_syscall:
  ; restore the RBP register
  pop rbp

  ; restore the RCX and R11 registers
  pop r11
  pop rcx

  ; mask interrupts again, for the same race condition reasons
  ;cli

  ; switch back to the user stack
  mov r12, [gs:16]   ; find current thread_t
  mov rsp, [r12 + 8] ; load original RSP from thread->syscall_rsp
 
  ; switch the GS base to the user's
  swapgs

  ; return to user long mode
  o64 sysret

; we 'fake' an interrupt for certain system calls
;
; this code is mostly identical to that in arc/intr/stub.s, with the following
; changes:
;
;   - we artificially push RIP, CS, RFLAGS, RSP and SS
;
;   - we push a fake error code/interrupt id
;
;   - we clear the interrupt flag (so it behaves like all of the other
;     'real' interrupts)
;
;   - we have some code to fix up the value in R11 (the MSB, which we know must
;     always be set for it to be a valid function pointer in kernel space, is
;     re-used as a flag that was clear, so we must set it to make the pointer
;     valid)
;
;   - we don't need to call SWAPGS upon entry (the SYSCALL entry code has
;     already done it)
;
;   - we don't need to call CLD (the SYSCALL instruction leaves the flag in a
;     well-defined state)
;
;   - we call R11 (the function pointer to the syscall) instead of intr_dispatch
.faux_intr:
  ; mask interrupts
  ;cli

  ; push state the processor automatically pushes during an interrupt
  mov rax, rsp
  push 0x10 ; SLTR_KERNEL_DATA | RPL0 - see arc/cpu/gdt.h
  push rax
  pushf
  push 0x08 ; SLTR_KERNEL_CODE | RPLO
  mov rax, qword .faux_intr_exit
  push rax

  ; push fake error code/interrupt id
  push 0    ; zero error code
  push 0xFA ; NOT_INTR - see arc/intr/common.h

  ; set the MSB in the function pointer so it becomes a real pointer again
  mov rax, SYSCALL_FAST
  or r11, rax

  ; save the register file
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rbp
  push rdi
  push rsi
  push rdx
  push rcx
  push rbx
  push rax

  ; increment mask count
  inc qword [gs:8]

  ; call the system call routine
  mov rdi, rsp ; first argument points to the processor state
  mov rbp, 0   ; terminate stack traces here
  call r11

  ; decrement mask count
  dec qword [gs:8]

  ; check if we are switching from supervisor to user mode
  mov rax, [rsp + 152]
  and rax, 0x3000      ; Ring 3 = 0x3***, Ring 0 = 0x0***
  jz .supervisor_exit

  ; switch back to the user's GS base if we are going from supervisor to user mode
  swapgs

.supervisor_exit:
  ; restore the register file
  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  pop rbp
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15

  ; pop the error code and interrupt id
  add rsp, 16

  ; return
  ; If the current thread did not change:
  ;    This jumps to .faux_intr_exit, the IF flag will stay cleared (it is not set in the pushed RFLAGS)
  ; If the current thread did change:
  ;    This continues execution either
  ;    - directly (if the thread was interrupted)
  ;    - in .faux_intr_exit (if the thread suspended itself using a syscall)
  iretq 

.faux_intr_exit:
  ; re-enable interrupts (the SYSCALL exit routine expects it), and then dive
  ; back into the SYSCALL exit routine
  ;sti
  swapgs
  jmp .post_syscall
