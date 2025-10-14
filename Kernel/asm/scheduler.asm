GLOBAL idle_process
GLOBAL set_process_stack
GLOBAL force_context_switch

EXTERN scheduler

SECTION .text

;===========================================================
; idle_process: Proceso idle que corre cuando no hay otros
;===========================================================
idle_process:
.loop:
    hlt                ; Espera a la siguiente interrupcion
    jmp .loop

;===========================================================
; set_process_stack: Configura el stack inicial de un proceso
; Parametros:
;   - rdi: argc
;   - rsi: argv
;   - rdx: stack base (top of stack)
;   - rcx: entry point (RIP)
; Retorna: RSP inicial del proceso
;===========================================================
set_process_stack:
    push rbp
    mov rbp, rsp
    
    ; Guardar stack actual
    mov r8, rsp
    
    ; Cambiar al nuevo stack
    mov rsp, rdx
    
    ; Construir stack frame como si fuera una interrupcion
    ; Orden: SS, RSP, RFLAGS, CS, RIP, registros
    
    push 0x00           ; SS (not used in 64-bit)
    push rdx            ; RSP
    push 0x202          ; RFLAGS (interrupts enabled)
    push 0x08           ; CS (kernel code segment)
    push rcx            ; RIP (entry point)
    
    ; Registros (en orden de pushState)
    push 0x00           ; rax
    push 0x00           ; rbx
    push 0x00           ; rcx
    push 0x00           ; rdx
    push 0x00           ; rbp
    push rdi            ; rdi = argc
    push rsi            ; rsi = argv
    push 0x00           ; r8
    push 0x00           ; r9
    push 0x00           ; r10
    push 0x00           ; r11
    push 0x00           ; r12
    push 0x00           ; r13
    push 0x00           ; r14
    push 0x00           ; r15
    
    ; Guardar el RSP resultante
    mov rax, rsp
    
    ; Restaurar el stack original
    mov rsp, r8
    
    pop rbp
    ret

;===========================================================
; force_context_switch: Fuerza un cambio de contexto
; (usado por yield syscall)
;===========================================================
force_context_switch:
    int 0x20            ; Trigger timer interrupt
    ret

; Seccion para indicar que el stack NO es ejecutable (seguridad)
section .note.GNU-stack noalloc noexec nowrite progbits

