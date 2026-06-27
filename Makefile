# Makefile
ASM = nasm
CC = gcc
LD = ld
CFLAGS = -m32 -fno-pie -ffreestanding -c
LDFLAGS = -m elf_i386 -T linker.ld

# Build both formats by default
all: floppy.bin USB-CD.iso

# Bootloader
boot.bin: boot.asm disk.asm gdt.asm pm-switch.asm
	$(ASM) -f bin boot.asm -o boot.bin

# Kernel entry object
kernel_entry.o: kernel_entry.asm
	$(ASM) -f elf32 kernel_entry.asm -o kernel_entry.o

# Kernel C object
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

# Link kernel ELF
kernel.bin: kernel_entry.o kernel.o
	$(LD) $(LDFLAGS) -o kernel.elf kernel_entry.o kernel.o
	objcopy -O binary kernel.elf kernel.bin

# Build raw floppy disk image (.bin format preserved)
floppy.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > floppy.bin
	# Pad to exactly 1.44MB (1440 KB) to satisfy strict El Torito floppy emulation size constraints
	truncate -s 1440k floppy.bin

# Build bootable ISO image (.iso support added)
USB-CD.iso: floppy.bin
	mkdir -p iso_root
	cp floppy.bin iso_root/
	# The boot image pathname (-b) must be relative to the source master directory (iso_root)
	mkisofs -b floppy.bin -c boot.catalog -o USB-CD.iso iso_root/

# Default run target aliases to floppy simulation
run: run-floppy

# Emulate raw floppy mode
run-floppy: floppy.bin
	qemu-system-i386 -fda floppy.bin -usb -device usb-tablet -d int -D qemu.log

# Emulate CD-ROM mode
run-iso: USB-CD.iso
	qemu-system-i386 -cdrom USB-CD.iso -usb -device usb-tablet -d int -D qemu.log

# Clean up workspace completely
clean:
	rm -f *.bin *.o *.elf floppy.bin USB-CD.iso
	rm -rf iso_root
