; gdt.asm
gdt_start:

gdt_null:                    ; mandatory blank entry
    dd 0x0
    dd 0x0

gdt_code:                    ; standard flat 4GB code segment descriptor
    dw 0xFFFF               ; Limit (bits 0-15)
    dw 0x0                  ; Base (bits 0-15)
    db 0x0                  ; Base (bits 16-23)
    db 10011010b            ; Access byte
    db 11001111b            ; Flags + Limit (bits 16-19)
    db 0x0                  ; Base (bits 24-31)

gdt_data:                    ; standard flat 4GB data segment descriptor
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; size limitation
    dd gdt_start                 ; linear layout base address

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
