; kernel_entry.asm
[bits 32]
[extern kernel_main]
global start

start:
    ; Synchronize all 32-bit Protected Mode Segment registers 
    mov ax, 0x10             ; 0x10 points to the GDT Data Segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000         ; Re-initialize safe, dedicated high-boundary stack top

    ; BARE-METAL PROTECTION SHIELD: Fully mask out the legacy 8259 hardware PICs
    ; This ensures spurious hardware interrupts will not trigger triple faults.
    mov al, 0xFF
    out 0x21, al             ; Mask Master PIC
    out 0xA1, al             ; Mask Slave PIC

    ; Push the safe EBX register value onto the stack.
    ; This passes the Framebuffer address as the first argument to kernel_main(uint32_t* fb)
    push ebx                 
    
    call kernel_main         ; Jump directly into your compiled C kernel code
    
    cli
    hlt                      ; Absolute catch loop safety sequence if kernel ever returns
    jmp $
