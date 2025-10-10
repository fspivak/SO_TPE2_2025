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

; Process syscalls
GLOBAL create_process
GLOBAL getpid
GLOBAL kill_process
GLOBAL block_process
GLOBAL unblock_process
GLOBAL set_process_priority
GLOBAL yield_process
GLOBAL ps_process
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
    cli
    hlt
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

; ===== PROCESS SYSCALLS =====

create_process:
    push rbp
    mov rbp, rsp
    mov rax, 60
    int 80h
    mov rsp, rbp
    pop rbp
    ret

getpid:
    push rbp
    mov rbp, rsp
    mov rax, 61
    int 80h
    mov rsp, rbp
    pop rbp
    ret

kill_process:
    push rbp
    mov rbp, rsp
    mov rax, 62
    int 80h
    mov rsp, rbp
    pop rbp
    ret

block_process:
    push rbp
    mov rbp, rsp
    mov rax, 63
    int 80h
    mov rsp, rbp
    pop rbp
    ret

unblock_process:
    push rbp
    mov rbp, rsp
    mov rax, 64
    int 80h
    mov rsp, rbp
    pop rbp
    ret

set_process_priority:
    push rbp
    mov rbp, rsp
    mov rax, 65
    int 80h
    mov rsp, rbp
    pop rbp
    ret

yield_process:
    push rbp
    mov rbp, rsp
    mov rax, 66
    int 80h
    mov rsp, rbp
    pop rbp
    ret

ps_process:
    push rbp
    mov rbp, rsp
    mov rax, 67
    int 80h
    mov rsp, rbp
    pop rbp
    ret

section .bss
registros resq 13
