; Bootloader that switches to protected mode and loads kernel
; Assemble with: nasm -f bin boot.asm -o boot.bin

[BITS 16]
[ORG 0x7C00]

start:
    ; Set up segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Print loading message
    mov si, msg_loading
    call print_string

    ; Reset disk system first
    mov ah, 0x00
    mov dl, 0x80        ; Drive 0
    int 0x13
    
    ; Load kernel from disk
    mov ah, 0x02        ; BIOS read sector function
    mov al, 15          ; Number of sectors to read
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start from sector 2
    mov dh, 0           ; Head 0
    mov dl, 0x80        ; Drive 0 (important!)
    mov bx, 0x1000      ; Load kernel at 0x1000
    int 0x13            ; Call BIOS disk interrupt
    
    jc disk_error

    ; Print success message
    mov si, msg_success
    call print_string

    ; Disable interrupts
    cli

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Enable protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code
    jmp 0x08:protected_mode

disk_error:
    mov si, msg_error
    call print_string
    jmp $

print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

msg_loading db 'Loading kernel...', 13, 10, 0
msg_success db 'Kernel loaded! Starting...', 13, 10, 0
msg_error db 'Disk read error!', 13, 10, 0

; GDT (Global Descriptor Table)
gdt_start:
    ; Null descriptor
    dd 0x0
    dd 0x0

    ; Code segment descriptor
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

    ; Data segment descriptor
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                 ; Offset

[BITS 32]
protected_mode:
    ; Set up segment registers for protected mode
    mov ax, 0x10    ; Data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000  ; Set up stack

    ; Jump to kernel
    jmp 0x1000

; Fill rest of bootloader with zeros and add boot signature
times 510-($-$$) db 0
dw 0xAA55
