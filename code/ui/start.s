; UI process main file.

[extern main]

[section .text]
[global _start]
_start:
  ; Jump to main function
  call main
  
  ; Run endlessly
  ; TODO proper process exit
  jmp $