
[global xsave]
xsave:
	; Save x87, SSE and AVX state
	mov eax, 0x00000007
	xor edx, edx
	xsave64 [rdi]
	
	; Done
	ret
	
[global xrstor]
xrstor:
	; Restore x87, SSE and AVX state
	mov eax, 0x00000007
	xor edx, edx
	xrstor64 [rdi]
	
	; Done
	ret
	
