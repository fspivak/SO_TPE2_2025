GLOBAL cpuVendor
GLOBAL getSeconds
GLOBAL getMinutes
GLOBAL getHours
GLOBAL getDoW
GLOBAL getMonth
GLOBAL getYear
GLOBAL outb
GLOBAL inb
GLOBAL returnKBOutputInterrupt

EXTERN esc_pressed

section .text
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid


	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret

returnKBOutputInterrupt:
	push rbp
	mov rbp, rsp

    mov rax, 0

.cicle:
	in al, 64h
    and al, 0x01
    je .cicle
    in al, 60h

	cmp al, 1
	jne .fin
	call esc_pressed
.fin:
	mov rsp, rbp
	pop rbp
    ret


getSeconds:
	push rbp
	mov rbp, rsp

	mov al, 0
	out 70h, al
	in al,71h

	mov rsp, rbp
	pop rbp
	ret

getMinutes:
	push rbp
	mov rbp, rsp

	mov al, 2
	out 70h, al
	in al,71h

	mov rsp, rbp
	pop rbp
	ret

getHours:
	push rbp
	mov rbp, rsp

	mov al, 4
	out 70h, al
	in al,71h

	mov rsp, rbp
	pop rbp
	ret

getDoW:
	push rbp
	mov rbp, rsp

	mov al, 7
	out 70h, al
	in al,71h

	mov rsp, rbp
	pop rbp
	ret

getMonth:
	push rbp
	mov rbp, rsp

	mov al, 8
	out 70h, al
	in al,71h

	mov rsp, rbp
	pop rbp
	ret

getYear:
	push rbp
	mov rbp, rsp

	mov al, 9
	out 70h, al
	in al,71h

	mov rsp, rbp
	pop rbp
	ret
outb:
	; Escribe un byte pasado como paramentro en el puerto
   	mov dx, di
    mov al, sil
    out dx, al 
	ret
inb:
	; Lee un byte pasado como paramentro del puerto
	mov dx, di
    in al, dx
	ret

; Seccion para indicar que el stack NO es ejecutable (seguridad)
section .note.GNU-stack noalloc noexec nowrite progbits