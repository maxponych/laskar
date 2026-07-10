global kb_stub
extern kb_handler

section .text
bits 64

kb_stub:
    push qword 0
    
    push qword 33
    
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    mov rdi, rsp
    call kb_handler
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    add rsp, 16
    
    mov  bl, 0x20
    xchg al, bl
    out  0x20, al
    xchg al, bl

    
    iretq
