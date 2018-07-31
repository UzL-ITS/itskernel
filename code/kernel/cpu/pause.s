[global pause_once]
pause_once:
  push rbp
  mov rbp, rsp
  pause
  pop rbp
  ret
