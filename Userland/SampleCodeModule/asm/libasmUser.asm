GLOBAL write
GLOBAL read
GLOBAL sleep
GLOBAL zoom
GLOBAL draw
GLOBAL screenDetails
GLOBAL setCursor
GLOBAL clearScreen
;GLOBAL devolverRegistros
GLOBAL getClock
GLOBAL playSound
GLOBAL getMiliSecs
GLOBAL getcharNL
GLOBAL rompeOpcode
GLOBAL impRegs
GLOBAL exit
GLOBAL malloc
GLOBAL free
GLOBAL memStatus
; EXTERN printRegistros

section .text

write:
    push rbp
    mov rbp,rsp
    mov rax, 1
    int 80h
    mov rsp, rbp
    pop rbp
    ret

read:
    push rbp
    mov rbp,rsp

    mov rax, 0
    int 80h
    
    mov rsp, rbp
    pop rbp
    ret
    

sleep:
    push rbp
    mov rbp,rsp
    mov rax, 35;
    int 80h
    mov rsp, rbp
    pop rbp
    ret

zoom:
    push rbp
    mov rbp, rsp
    mov rax, 28
    int 80h
    mov rsp, rbp
    pop rbp
    ret

draw:
    push rbp
    mov rbp, rsp
    mov rax, 43 ;franco colapinto
    int 80h
    mov rsp, rbp
    pop rbp
    ret

screenDetails:
    push rbp
    mov rbp, rsp
    mov rax, 44
    int 80h
    mov rsp, rbp
    pop rbp
    ret

setCursor:
    push rbp
    mov rbp, rsp
    mov rax, 45
    int 80h
    mov rsp, rbp
    pop rbp
    ret

clearScreen:
    push rbp
    mov rbp, rsp
    mov rax, 48
    int 80h
    mov rsp, rbp
    pop rbp
    ret

getClock:
    push rbp
    mov rbp, rsp
    mov rax, 46
    int 80h
    mov rsp, rbp
    pop rbp
    ret

playSound:
    push rbp 
    mov rbp, rsp 
    mov rax, 47
    int 80h
    mov rsp, rbp
    pop rbp
    ret

getMiliSecs:
    push rbp
    mov rbp, rsp

    mov rax, 87
    int 80h

    mov rsp, rbp
    pop rbp
    ret

getcharNL:
    push rbp
    mov rbp, rsp

    mov rax, 2
    int 80h

    mov rsp, rbp
    pop rbp
    ret

impRegs:
    push rbp
    mov rbp,rsp

    mov rax, 12 ;boca
    int 80h

    mov rsp, rbp
    pop rbp
    ret

rompeOpcode:
    ud2
    ret

exit:
    mov rax, 68      ; Syscall exit
    int 80h
    ret

malloc:
    push rbp
    mov rbp, rsp
    mov rax, 50
    int 80h
    mov rsp, rbp
    pop rbp
    ret

free:
    push rbp
    mov rbp, rsp
    mov rax, 51
    int 80h
    mov rsp, rbp
    pop rbp
    ret

memStatus:
    push rbp
    mov rbp, rsp
    mov rax, 52
    int 80h
    mov rsp, rbp
    pop rbp
    ret

;======== Syscall generica ========

GLOBAL sys_call
; @param id: rdi
; @param arg1: rsi
; @param arg2: rdx
; @param arg3: rcx
; @param arg4: r8
; @param arg5: r9
; @param arg6: [rbp+16] (stack)
; @return: result of the syscall
; int64_t sys_call(uint64_t id, uint64_t arg1, uint64_t arg2, 
;                  uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6)
sys_call:
    push rbp
    mov rbp, rsp
    
    mov rax, rdi        ; syscall id
    mov rdi, rsi        ; arg1
    mov rsi, rdx        ; arg2
    mov rdx, rcx        ; arg3
    mov rcx, r8         ; arg4
    mov r8,  r9         ; arg5
    mov r9,  [rbp+16]   ; arg6 (stack)
    
    int 80h
    
    mov rsp, rbp
    pop rbp
    ret

section .bss
registros resq 13

; Seccion para indicar que el stack NO es ejecutable (seguridad)
section .note.GNU-stack noalloc noexec nowrite progbits
