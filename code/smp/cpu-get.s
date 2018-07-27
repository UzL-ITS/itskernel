
[global cpu_get]
cpu_get:
  mov rax, [gs:0] ; the first entry in the struct points to itself
  ret
