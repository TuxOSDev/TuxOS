# TuxOS

**A tiny 32-bit operating system with a custom graphical window stack and retro games written from scratch.**

## About

TuxOS is a minimal 32‑bit hobby operating system for x86 architecture. It boots into a 32-bit linear framebuffer graphical user interface (GUI) using VBE (VESA Bios Extensions). It features its own bootloader, a double-buffered graphic stack, full font rendering, a PS/2 mouse polling engine with window dragging mechanics, a PS/2 keyboard driver, and an interactive shell terminal with built-in games.

**Version:** 0.2.4

## Features

- **Custom Bootloader:** Assembles into the first sector (512 bytes), loads the kernel into memory via BIOS interrupts, and safely transitions the CPU into 32-bit protected mode.
- **Graphical Stack:** Operates at 800x600 resolution with 32-bit depth using standard VBE. Employs assembly-optimized double-buffering (`rep movsl` / `rep stosl`) for smooth UI drawing and selective bounding box updates.
- **Window Management & Mouse Support:** Low-level PS/2 mouse driver with real-time cursor rendering and manual click-and-drag handling to reposition active workspace windows.
- **Apps Menu:** Desktop bar featuring dropdown application hooks to cleanly open and interface with active modules.
- **Interactive Terminal:** Persistent history buffer supporting custom user commands:
  - `help` – list all available interactive utilities
  - `uname` – outputs system details (`TuxOS v0.2.4`)
  - `clear` – flushes the terminal history view
  - `echo <text>` – repeat user string back to stdout
  - `rand` – calculates a pseudo-random integer
  - `time` – displays simulated system uptime ticks
  - `calc <n1> <op> <n2>` – simple 32-bit integer operations (+, -, *, /)
  - `mdump <hex_addr>` – inspects raw contents of low-level kernel memory
  - `game` – interactive "Guess the Number" shell game
  - `pong` – launches a state-based arcade mode within the window space
  - `shutdown` – cleanly triggers QEMU/VirtualBox ACPI/APM power-off traps
- **Automatic CI Testing:** Configured for automated compilation and checkouts using GitHub Actions.

## Building

### Requirements

- Linux environment (or WSL on Windows)
- `nasm` (assembler)
- `gcc` with 32‑bit support (`gcc‑multilib` or specialized cross‑compiler toolchains)
- `ld` (GNU linker)
- `make`
- `qemu-system-i386` (for local simulation testing)

Install dependencies on Debian/Ubuntu/WSL:

```bash
sudo apt update
sudo apt install nasm gcc-multilib build-essential qemu-system-x86 make

```

### Build & Run

```bash
make clean
make
make run

```

The automation script cleans intermediate objects, builds the custom boot block, compiles the C source code stack, generates a combined bootable image file (`os-image.bin`), and triggers QEMU execution.

## Creating a USB Bootable Image

To load the system binary onto bare-metal x86 environments, flash the output direct to an available block device storage vector:

```bash
sudo dd if=os-image.bin of=/dev/sdX bs=512 count=2880 status=progress
sync

```

*(Ensure you substitute `/dev/sdX` with your exact destination drive designation path before initializing).*

## File Overview

| File | Purpose |
| --- | --- |
| `boot.asm` | First‑stage bootloader initialization block |
| `disk.asm` | Low-level BIOS disk read execution routine |
| `gdt.asm` | Custom Global Descriptor Table structure mappings |
| `pm-switch.asm` | Secure real-mode to 32‑bit protected-mode pipeline transition |
| `kernel_entry.asm` | Main assembly entry point configuring stack frames |
| `kernel.c` | Core monolithic kernel: VBE stack, mouse/keyboard drivers, GUI apps, games, shell |
| `linker.ld` | Linker map script binding the executable base location at 0x10000 |
| `Makefile` | Build automation chain script |
| `.github/workflows/ci.yml` | GitHub Actions automated integration configuration |

## License

This project is licensed under the **GNU General Public License v2.0** – see the [LICENSE](https://www.google.com/search?q=LICENSE) file for details.

The embedded 8x8 font bitmap implementation handles core pixel glyph representations for terminal workspace layouts.

## Author & Credits

TuxOS is a hobby learning project focused on low-level system design and operating system architecture.

Inspired by standard community OS-dev documentation and code layouts. Special thanks to the osdev.org community.

---

**Warning:** This repository represents a personal, early-stage learning environment. There is no active kernel scheduler, virtual file-system layer, or hardware page isolation. Run on legacy test machines or virtualization suites at your own discretion.
