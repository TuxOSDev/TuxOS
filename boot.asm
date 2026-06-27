; boot.asm -------------------------------------------------------------
[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x1000          ; Kernel loaded at segment 0x1000 (0x10000 physical) [cite: 774, 775]

start:
    ; 1. Sanitize segment registers immediately for real hardware stability 
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov bp, 0x9000
    mov sp, bp
    sti

    mov [BOOT_DRIVE], dl             ; Safely save boot drive using sanitized DS 

    call load_kernel                 ; Loads kernel (changes ES to 0x1000) 
    call set_vbe_graphics            ; Query and activate 800x600x32 bpp dynamically 
    call switch_to_pm                ; Jump to 32-bit Protected Mode 
    jmp $

load_kernel:
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000
    mov dh, 40
    mov dl, [BOOT_DRIVE]
    call disk_load                   ; Read kernel tracks into 0x10000 
    ret

; Safe absolute addresses in conventional memory (completely outside our boot sector space)
VBE_CTRL_BLOCK equ 0x7E00        ; 512 bytes: 0x7E00 - 0x7FFF
VBE_MODE_BLOCK equ 0x8000        ; 256 bytes: 0x8000 - 0x80FF

set_vbe_graphics:
    pusha
    
    ; 2. Reset DS and ES to 0 to safely target our variables and VBE structures 
    xor ax, ax
    mov es, ax
    mov ds, ax

    ; 3. Query VBE Controller Information 
    mov ax, 0x4F00
    mov di, VBE_CTRL_BLOCK
    mov dword [di], "VESA"       ; Pre-fill signature (required by VBE 2.0+)
    int 0x10
    cmp ax, 0x004F
    jne vbe_failed

    ; 4. Extract the far pointer to the video mode list 
    mov si, [VBE_CTRL_BLOCK + 14]    ; Offset of mode list
    mov ax, [VBE_CTRL_BLOCK + 16]    ; Segment of mode list
    mov fs, ax                       ; Load list segment into FS

.next_mode:
    ; Read the mode entry from the list (FS:SI)
    mov cx, [fs:si]
    cmp cx, 0xFFFF                   ; 0xFFFF marks the end of the VBE mode list
    je vbe_failed

    push si                          ; Preserve our current list pointer position

    ; 5. Query details for this specific mode number 
    mov ax, 0x4F01
    mov di, VBE_MODE_BLOCK
    int 0x10
    cmp ax, 0x004F
    jne .skip_mode

    ; Validate that the mode matches your kernel setup requirements:
    ; Check X resolution == 800 
    cmp word [VBE_MODE_BLOCK + 18], 800
    jne .skip_mode

    ; Check Y resolution == 600 
    cmp word [VBE_MODE_BLOCK + 20], 600
    jne .skip_mode

    ; Check Bits Per Pixel == 32 
    cmp byte [VBE_MODE_BLOCK + 25], 32
    jne .skip_mode

    ; Check if Linear Frame Buffer (LFB) is supported (Bit 7 of ModeAttributes) [cite: 752, 779]
    mov ax, [VBE_MODE_BLOCK + 0]
    test ax, 0x0080
    jz .skip_mode

    ; Mode Match Successful! Extract 32-bit physical address of the LFB [cite: 752, 778]
    mov eax, [VBE_MODE_BLOCK + 0x28]
    mov [fb_pointer], eax            ; Pass to kernel entry 

    ; Activate the selected VBE Mode [cite: 779]
    mov bx, cx
    or bx, 0x4000                    ; Turn on Bit 14 to enable the Linear Frame Buffer layout [cite: 779]
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne vbe_failed

    pop si                           ; Balance the stack
    popa
    ret

.skip_mode:
    pop si                           ; Restore list position
    add si, 2                        ; Move to the next 16-bit mode entry
    jmp .next_mode

vbe_failed:
    ; Print "VE" on the screen and halt if graphics fail 
    mov ah, 0x0e
    mov al, 'V'
    int 0x10
    mov al, 'E'
    int 0x10
    cli
    hlt

%include "disk.asm"                  ; 
%include "gdt.asm"                   ; 
%include "pm-switch.asm"             ; 

BOOT_DRIVE db 0                      ; 

align 4
fb_pointer dd 0                      ; 

; Pad out the boot sector to exactly 512 bytes [cite: 754, 780]
times 510 - ($ - $$) db 0            ; 
dw 0xaa55                            ;
