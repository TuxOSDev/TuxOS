; pm-switch.asm
[bits 16]
switch_to_pm:
    cli                      ; Disable interrupts
    lgdt [gdt_descriptor]    ; Load GDT

    mov eax, cr0
    or eax, 0x1              ; Set protected mode bit
    mov cr0, eax

    jmp dword CODE_SEG:init_pm     ; Far jump to clear pipeline

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000         ; Stack layout pointer setup
    mov esp, ebp

    call BEGIN_PM            

BEGIN_PM:
    ; Read the dynamic VRAM pointer resolved by the BIOS
    mov eax, [fb_pointer]

    ; Jump directly to our kernel start address at 0x10000
    call 0x10000
    jmp $
