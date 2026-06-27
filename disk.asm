; disk.asm
[bits 16]
disk_load:
    pusha
    mov word [sectors_left], 40       ; Total 40 sectors to fetch (Kernel allocation)
    mov byte [cur_sector], 2          ; Begin immediately at sector 2
    mov byte [cur_head], 0            ; Head 0
    mov byte [cur_cylinder], 0        ; Cylinder 0

.read_loop:
    cmp word [sectors_left], 0
    je .read_done

    mov ah, 0x02                      ; BIOS read sector function
    mov al, 1                         ; Read strictly 1 sector to protect track bounds
    mov ch, [cur_cylinder]
    mov dh, [cur_head]
    mov cl, [cur_sector]
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .disk_retry                    ; Fallback to error mitigation on fault
    jmp .read_success

.disk_retry:
    ; Reset the physical disk controller state machine
    xor ax, ax
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    ; Re-attempt identical sector read operations
    mov ah, 0x02
    mov al, 1
    mov ch, [cur_cylinder]
    mov dh, [cur_head]
    mov cl, [cur_sector]
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error                     ; Hard failure trap

.read_success:
    ; Advance target segment address pointer by 512 bytes (0x0020 paragraphs)
    ; This prevents 16-bit register offset overflows (64KB boundary crashes)
    mov ax, es
    add ax, 0x0020
    mov es, ax

    dec word [sectors_left]           ; Reduce tracking counter

    ; Increment CHS variables mapped to safe 1.44MB physical parameters
    inc byte [cur_sector]
    cmp byte [cur_sector], 18         ; Track limit bounds check
    jbe .read_loop

    mov byte [cur_sector], 1          ; Reset to sector 1
    inc byte [cur_head]               ; Flip storage head index
    cmp byte [cur_head], 1            ; Dual head boundary check
    jbe .read_loop

    mov byte [cur_head], 0            ; Reset to head 0
    inc byte [cur_cylinder]           ; Increment to next sequential track cylinder
    jmp .read_loop

.read_done:
    popa
    ret

disk_error:
    mov si, ERROR_MSG
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp print_string
.done:
    ret

ERROR_MSG db "Disk read error", 0

; Thread-safe state variables isolated from BIOS register clobbering
sectors_left dw 0
cur_sector   db 0
cur_head     db 0
cur_cylinder db 0
