
GLOBAL _cli
GLOBAL _sti
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL haltcpu
GLOBAL _hlt
GLOBAL _force_scheduler_interrupt
GLOBAL manually_triggered_timer_interrupt

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq02Handler
GLOBAL _irq03Handler
GLOBAL _irq04Handler
GLOBAL _irq05Handler
GLOBAL _irq60Handler

GLOBAL _exception0Handler
GLOBAL _exception6Handler

GLOBAL esc_pressed
GLOBAL get_regs



EXTERN irqDispatcher
EXTERN exceptionDispatcher
EXTERN syscallDispatcher
EXTERN scheduler

EXTERN getStackBase
EXTERN printRegistros



SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popState 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro pushStateNoRax 0
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popStateNoRax 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
%endmacro

%macro irqHandlerMaster 1
	pushState
	mov byte [regs_save], 0
	mov rbp, rsp
	mov rdi, %1 ; pasaje de parametro
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	cmp byte [regs_save], 1
	jne .fin
	mov [registros],rax
    mov rax,registros
    add rax, 8
    mov [rax], rbx
    add rax, 8
    mov [rax], rcx
    add rax, 8
    mov [rax], rdx
    add rax, 8
    mov [rax], rsi
    add rax, 8
    mov [rax], rdi
    add rax, 8
    mov [rax], rbp
    add rax, 8
    mov [rax], rsp
    add rax, 8
    mov [rax], r8
    add rax, 8
    mov [rax], r9
	add rax, 8
	mov rdi, [rsp] ;RIP
	mov [rax], rdi
	add rax,8
	mov rdi,[rsp+8] ;CS
	mov [rax],rdi
	add rax,8
	mov rdi,[rsp+8*2] ;RFLAGS
	mov [rax],rdi
.fin:
	iretq
%endmacro



%macro exceptionHandler 1
	push rdi
	mov [registros],rax
    mov rax,registros
    add rax, 8
    mov [rax], rbx
    add rax, 8
    mov [rax], rcx
    add rax, 8
    mov [rax], rdx
    add rax, 8
    mov [rax], rsi
    add rax, 8
    mov [rax], rdi
    add rax, 8
    mov [rax], rbp
    add rax, 8
    mov [rax], rsp
    add rax, 8
    mov [rax], r8
    add rax, 8
    mov [rax], r9
	add rax, 8
	mov rbx, [rsp+8] ;RIP
	mov [rax], rbx
	add rax,8
	mov rbx,[rsp+8*2] ;CS
	mov [rax],rbx
	add rax,8
	mov rbx,[rsp+8*3] ;RFLAGS
	mov [rax],rbx

    mov rdi, registros
	call printRegistros
	pop rdi
	mov rdi, %1 ; pasaje de parametro
	call exceptionDispatcher
	call getStackBase
	mov [rsp+8*3], rax
	mov rax, userLand
	mov [rsp], rax 
	iretq
%endmacro


_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret


_sti:
	sti
	ret

picMasterMask:
	push rbp
    mov rbp, rsp
    mov ax, di
    out	21h,al
    pop rbp
    retn

picSlaveMask:
	push    rbp
    mov     rbp, rsp
    mov     ax, di  ; ax = mascara de 16 bits
    out	0A1h,al
    pop     rbp
    retn


;8254 Timer (Timer Tick) con scheduler
_irq00Handler:
	pushState
	mov rbp, rsp
	
	mov rdi, 0 ; parametro: IRQ number
	call irqDispatcher
	
	; Llamar al scheduler con el stack pointer actual
	mov rdi, rsp
	call scheduler

	cmp rax, 0
	je .scheduler_error
	
	mov rsp, rax  ; Cambiar al stack del siguiente proceso
	jmp .scheduler_ok
	
.scheduler_error:
	
.scheduler_ok:
	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al
	
	popState
	iretq

;Keyboard
_irq01Handler:
	irqHandlerMaster 1

;Cascade pic never called
_irq02Handler:
	irqHandlerMaster 2

;Serial Port 2 and 4
_irq03Handler:
	irqHandlerMaster 3

;Serial Port 1 and 3
_irq04Handler:
	irqHandlerMaster 4

;USB
_irq05Handler:
	irqHandlerMaster 5

_irq60Handler:
	push rbp
	mov rbp, rsp
	
	pushStateNoRax    ; No guardar RAX para preservar valor de retorno
	; - Primeros 6 argumentos: de registros (rdi, rsi, rdx, rcx, r8, r9)
	; - 7mo argumento en adelante: del stack (despues del return address)
	; Necesitamos preservar arg6 (r9) y ponerlo en el stack para que va_list lo lea
	push r9            ; Guardar arg6 en el stack (sera el 7mo argumento para va_list)
	
	mov r9, r8         ; r9 = arg5 (6to argumento)
	mov r8, rcx        ; r8 = arg4 (5to argumento)
	mov rcx, rdx       ; rcx = arg3 (4to argumento)
	mov rdx, rsi       ; rdx = arg2 (3er argumento)
	mov rsi, rdi       ; rsi = arg1 (2do argumento)
	mov rdi, rax       ; rdi = syscall id (1er argumento)
	
	; Ahora el stack tiene: [arg6] [return address] [otros...]
	; va_list leerá arg6 desde el stack correctamente
	
	call syscallDispatcher
	
	; Limpiar arg6 del stack (va_list ya lo leyó)
	add rsp, 8          ; Remover arg6 del stack
	
	popStateNoRax     ; No restaurar RAX, preserva el retorno de syscallDispatcher
	
	mov rsp, rbp
	pop rbp
	iretq

;Zero Division Exception
_exception0Handler:
	exceptionHandler 0

_exception6Handler:
	exceptionHandler 6


haltcpu:
	cli
	hlt
	ret

esc_pressed:
	mov byte [regs_save], 1
	ret

get_regs:
	mov rax, registros
	ret

_force_scheduler_interrupt:
	mov BYTE [manually_triggered_timer_interrupt], 0x01
	int 0x20
	ret


SECTION .data
userLand equ 0x400000

SECTION .bss
	aux resq 1
	registros resq 13
	regs_save resb 0
	manually_triggered_timer_interrupt resb 1

; Seccion para indicar que el stack NO es ejecutable (seguridad)
section .note.GNU-stack noalloc noexec nowrite progbits
