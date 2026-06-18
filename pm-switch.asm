; pm-switch.asm
[bits 16]
switch_to_pm:
    cli                      ; disable interrupts
    lgdt [gdt_descriptor]    ; load GDT

    mov eax, cr0
    or eax, 0x1              ; set protected mode bit
    mov cr0, eax

    jmp dword CODE_SEG:init_pm     ; far jump to clear pipeline

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000         ; new stack at top of free space
    mov esp, ebp

    call BEGIN_PM            ; call kernel entry point

BEGIN_PM:
    ; Read the physical frame buffer pointer we saved in the boot sector
    mov eax, [0x7C00 + fb_pointer]
    
    ; Jump exactly to our kernel start address at 0x10000
    call 0x10000
    jmp $                    ; infinite loop if kernel returns
