global loader
extern main
extern initializeKernelBinary

loader:
	call initializeKernelBinary	; Set up the kernel binary, and get thet stack address
	mov rsp, rax				; Set up the stack with the returned address
	call main
hang:
	cli
	hlt	; halt machine should kernel return
	jmp hang

; Seccion para indicar que el stack NO es ejecutable (seguridad)
section .note.GNU-stack noalloc noexec nowrite progbits
