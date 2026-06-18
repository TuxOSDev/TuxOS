; boot.asm -------------------------------------------------------------
[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x1000          ; kernel loaded at 0x10000

start:
    mov [BOOT_DRIVE], dl
    mov bp, 0x9000
    mov sp, bp

    call load_kernel
    call set_vbe_graphics        ; Query and activate 800x600x32 bpp
    call switch_to_pm
    jmp $

load_kernel:
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    mov dh, 40
    mov dl, [BOOT_DRIVE]
    call disk_load
    ret

set_vbe_graphics:
    pusha
    ; Get Mode Information for 800x600x32 (Mode 0x118)
    mov ax, 0x4F01
    mov cx, 0x118
    mov di, vbe_info_block
    int 0x10
    cmp ax, 0x004F
    jne vbe_failed

    ; Save the Linear Frame Buffer physical address from offset 40 (0x28)
    mov eax, [vbe_info_block + 0x28]
    mov [fb_pointer], eax

    ; Set VBE Mode 0x118 + Enable Linear Frame Buffer bit (0x4000)
    mov ax, 0x4F02
    mov bx, 0x4118
    int 0x10
    cmp ax, 0x004F
    jne vbe_failed

    popa
    ret

vbe_failed:
    ; Quick flashing background text fallback if VBE configuration isn't supported
    mov ah, 0x0e
    mov al, 'V'
    int 0x10
    mov al, 'E'
    int 0x10
    cli
    hlt

%include "disk.asm"
%include "gdt.asm"
%include "pm-switch.asm"

BOOT_DRIVE db 0

align 4
fb_pointer dd 0

align 4
vbe_info_block: times 256 db 0

times 510 - ($ - $$) db 0
dw 0xaa55
