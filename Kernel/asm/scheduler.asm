GLOBAL idle_process
GLOBAL set_process_stack
GLOBAL set_idle_stack
GLOBAL force_context_switch

EXTERN scheduler

SECTION .text

;===========================================================
; idle_process: Proceso idle que corre cuando no hay otros
;===========================================================
idle_process:
.loop:
    sti
    hlt                ; Espera a la siguiente interrupcion (halta CPU hasta que llegue IRQ)
    jmp .loop

;===========================================================
; set_idle_stack: Configura el stack inicial del proceso idle
; Parametros:
;   - rdi: stack base (top of stack)
;   - rsi: PCB pointer
; Retorna: RSP inicial del proceso idle
;===========================================================
set_idle_stack:
    push rbp
    mov rbp, rsp
    
    ; Guardar stack actual
    mov r8, rsp
    
    ; Cambiar al nuevo stack
    mov rsp, rdi
    and rsp, -16
    
    ; Construir stack frame como si fuera una interrupcion
    ; Orden: SS, RSP, RFLAGS, CS, RIP, registros
    
    push 0x00           ; SS (user data segment)
    push rdi            ; RSP (stack pointer)
    push 0x202          ; RFLAGS (interrupts enabled)
    push 0x08           ; CS (kernel code segment)
    push idle_process   ; RIP (salta directamente a idle_process, no a process_entry_wrapper)
    
    ; Registros (en orden de pushState)
    push 0x00           ; rax
    push 0x00           ; rbx
    push rsi            ; rcx = PCB pointer
    push rdi            ; rdx = stack pointer
    push 0x00           ; rbp
    push 0x00           ; rsi (no usado en idle)
    push 0x00           ; rdi (no usado en idle)
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
; set_process_stack: Configura el stack inicial de un proceso
; Parametros:
;   - rdi: argc
;   - rsi: argv
;   - rdx: stack base (top of stack)
;   - rcx: PCB pointer
; Retorna: RSP inicial del proceso
;===========================================================
set_process_stack:
    push rbp
    mov rbp, rsp
    
    ; Guardar stack actual
    mov r8, rsp
    
    ; Cambiar al nuevo stack
    mov rsp, rdx
    and rsp, -16
    
    ; Construir stack frame como si fuera una interrupcion
    ; Orden: SS, RSP, RFLAGS, CS, RIP, registros
    
    push 0x00           ; SS (user data segment)
    push rdx            ; RSP (stack pointer)
    push 0x202          ; RFLAGS (interrupts enabled)
    push 0x08           ; CS (kernel code segment)
    extern process_entry_wrapper
    push process_entry_wrapper ; RIP (wrapper function)
    
    ; Registros (en orden de pushState)
    push 0x00           ; rax
    push 0x00           ; rbx
    push rcx            ; rcx = PCB pointer
    push rdx            ; rdx = stack pointer
    push 0x00           ; rbp
    push rsi            ; rsi = argv
    push rdi            ; rdi = argc
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

