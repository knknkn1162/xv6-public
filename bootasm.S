#include "asm.h"
#include "memlayout.h"
#include "mmu.h"

# Start the first CPU: switch to 32-bit protected mode, jump into C.
# The BIOS loads this code from the first sector of the hard disk into
# memory at physical address 0x7c00 and starts executing in real mode
# with %cs=0 %ip=7c00.

.code16                       # Assemble for 16-bit mode
.globl start
# 0x7c00 ~ 0x7e00
# see also https://wiki.osdev.org/Memory_Map_(x86)#Overview
start:
  # when xv6 is ready, it will re-enable interrupts
  cli                         # BIOS enabled interrupts; disable

  # Zero data segment registers DS, ES, and SS.
  # BIOS doesnot guarantee anything about the contents of %ds, %es, %ss.
  xorw    %ax,%ax             # Set %ax to zero
  movw    %ax,%ds             # -> Data Segment
  movw    %ax,%es             # -> Extra Segment
  movw    %ax,%ss             # -> Stack Segment

  # Physical address line A20 is tied to zero so that the first PCs 
  # with 2 MB would run software that assumed 1 MB.  Undo that.
# The boot loader must enable the 21st address bit using I/O to the keyboard controller on ports 0x64 & 0x60
seta20.1:
  # read the status register form the keyboard controller via the port 0x64
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  # If input buffer is full(bit1 is set), repeat
  jnz     seta20.1

  # the CPU writes to port 0x64, the byte is interpreted as a command byte.
  # Write output port(P2) ..0xd1
  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  # Command 0xdf: Enable A20 address line
  movb    $0xdf,%al               # 0xdf -> port 0x60
  # If the CPU writes to port 0x60, the byte is interpreted as a data byte.
  # WHY?
  # suppose that It can be read/written by writing 0x20/0x60 to port 0x64 and then reading/writing a data byte from/to port 0x60.
  outb    %al,$0x60

  # Switch from real to protected mode.  Use a bootstrap GDT that makes
  # virtual addresses map directly to physical addresses so that the
  # effective memory map doesn't change during the transition.
  # Load the Global Descriptor Table with segment descriptors suitable for code, data, and stack.
  lgdt    gdtdesc
  movl    %cr0, %eax
  #define CR0_PE          0x00000001      // Protection Enable
  orl     $CR0_PE, %eax
  movl    %eax, %cr0

//PAGEBREAK!
  # Complete the transition to 32-bit protected mode by using a long jmp
  # to reload %cs and %eip.  The segment descriptors are set up with no
  # translation, so that the mapping is still the identity mapping.

  #define SEG_KCODE 1  // kernel code
  # cannot modify %cs directly, so instead the code executes an ljmp instruction
# set %cs <- $(SEG_CODE<<3) = 0x08, which is at the 2nd GDT entry
  ljmp    $(SEG_KCODE<<3), $start32

.code32  # Tell assembler to generate 32-bit code now.
start32:
  # Set up the protected-mode data segment registers
  #define SEG_KDATA 2  // kernel data+stack
  movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
  # %ds <- 0x10, which is the 3rd index GDT entry
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %ss                # -> SS: Stack Segment
  movw    $0, %ax                 # Zero segments not ready for use
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS

  # Set up the stack pointer and call into C.
  movl    $start, %esp
  call    bootmain

  # If bootmain returns (it shouldn't), trigger a Bochs
  # Bochs is a highly portable open source IA-32 (x86) PC emulator
  # breakpoint if running under Bochs, then loop.
  movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
  movw    %ax, %dx
  outw    %ax, %dx
  # return the Bochs to the debugger prompt. Basically the same as doing CTRL+C.
  movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
  outw    %ax, %dx
spin:
  jmp     spin

# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
  # each entry is 8 byte
  SEG_NULLASM                             # null seg 0x0000 0000 00 00 00 00
  SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
  SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg

gdtdesc:
  # The lower 16 bits tell the size of the GDT
  .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
  # the upper 32 bits tell the location of the GDT in memory
  .long   gdt                             # address gdt
