; kernel_entry.asm
[bits 32]
[extern kernel_main]
global start
start:
    mov esp, 0x90000        ; stack top
    push eax                ; Pass Framebuffer base pointer as the first parameter
    call kernel_main
    jmp $
