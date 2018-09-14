# The xv6 kernel starts executing in this file. This file is linked with
# the kernel C code, so it can refer to kernel symbols such as main().
# The boot block (bootasm.S and bootmain.c) jumps to entry below.
        
# Multiboot header, for multiboot boot loaders like GNU Grub.
# http://www.gnu.org/software/grub/manual/multiboot/multiboot.html
#
# Using GRUB 2, you can boot xv6 from a file stored in a
# Linux file system by copying kernel or kernelmemfs to /boot
# and then adding this menu entry:
#
# menuentry "xv6" {
# 	insmod ext2
# 	set root='(hd0,msdos1)'
# 	set kernel='/boot/kernel'
# 	echo "Loading ${kernel}..."
# 	multiboot ${kernel} ${kernel}
# 	boot
# }

#include "asm.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"

# Multiboot header.  Data to direct multiboot loader.
# The Multiboot header must be contained completely within the first 8192 bytes of the OS image, and must be longword (32-bit) aligned. In general, it should come as early as possible, and may be embedded in the beginning of the text segment after the real executable header.
.p2align 2
.text
.globl multiboot_header
multiboot_header:
  #define magic 0x1badb002
  #define flags 0

  # The field ‘magic’ is the magic number identifying the header, which must be the hexadecimal value 0x1BADB002. 
  .long magic

  # Naturally, all as-yet-undefined bits in the ‘flags’ word must be set to zero in OS images.
  .long flags
# when added to the other magic fields (i.e. ‘magic’ and ‘flags’), must have a 32-bit unsigned sum of zero.
  .long (-magic-flags)

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
# the entry address is 0x10000c
_start = V2P_WO(entry)

# Entering xv6 on boot processor, with paging off.
.globl entry
entry:
  # Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
# #define CR4_PSE         0x00000010      // Page size extension
  // Enables 4M byte pages when 32-bit paging when set. Note that the pages are not 4M byte without PTE_PS setting
  // restricts 32-bit paging to pages of 4K bytes when clear
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
  # Set page directory(tentative). Later, we are to set cr3 register by switchkvm() in vm.c
  // %cr3 must be a physical address
  // paging hardware doen't know how to translate virtual address yet
  // mov    $0x1020c0,%eax
  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3
  # Turn on paging.
  movl    %cr0, %eax
# #define CR0_PG          0x80000000      // Paging
# #define CR0_WP          0x00010000      // Write Protect
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0

  # Set up the stack pointer.
  # mov    $0x801020c0,%esp
  movl $(stack + KSTACKSIZE), %esp

  # Jump to main(), and switch to executing at
  # high addresses. The indirect call is needed because
  # the assembler produces a PC-relative instruction
  # for a direct jump.
  # mov    $0x80100034,%eax
  mov $main, %eax
  jmp *%eax

// declares a common symbol named symbol.
// malloc KSTACKSIZE
.comm stack, KSTACKSIZE
