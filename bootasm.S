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
  # The IF flag can be set or cleared with the STI (set interrupt-enable flag) and CLI (clear interrupt-enable flag) instructions, respectively.
  cli                         # BIOS enabled interrupts; disable

  # Zero data segment registers DS, ES, and SS.
  # BIOS doesnot guarantee anything about the contents of %ds, %es, %ss.
  xorw    %ax,%ax             # Set %ax to zero
  movw    %ax,%ds             # -> Data Segment
  movw    %ax,%es             # -> Extra Segment
  movw    %ax,%ss             # -> Stack Segment

  # Summarize) 8086(1MB memory) -> Intel286(16MB memory) .. mask A20 line by default because of compatibility
  # History) When the IBM-AT (Intel 286) was introduced, it was able to access up to sixteen megabytes of memory (instead of the 1 MByte of the 8086). But to remain compatible with the 8086, a quirk in the 8086 architecture (memory wraparound) had to be duplicated in the AT. To achieve this, the A20 line on the address bus was disabled by default. (see https://wiki.osdev.org/A20_Line )

  # Physical address line A20 is tied to zero so that the first PCs 
  # with 2 MB would run software that assumed 1 MB.  Undo that.
# The boot loader must enable the 21st address bit using I/O to the keyboard controller on ports 0x64 & 0x60
seta20.1:
  # read the status register form the keyboard controller via the port 0x64
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  # If input buffer is full(bit1 is set), repeat
  jnz     seta20.1

  # write output port. next byte written  to 0060 will be written to the 804x output port; the original IBM AT and many compatibles use bit 1 of the output port to control the A20 gate.
  # Write output port(P2) ..0xd1
  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  # Command 0xdf: Enable A20 address line
  movb    $0xdf,%al               # 0xdf -> port 0x60
  # sngl  enable address line A20
  outb    %al,$0x60

  # Switch from real to protected mode.  Use a bootstrap GDT that makes
  # virtual addresses map directly to physical addresses so that the
  # effective memory map doesn't change during the transition.
  # Load the Global Descriptor Table with segment descriptors suitable for code, data, and stack.
  lgdt    gdtdesc
  movl    %cr0, %eax
  # #define CR0_PE          0x00000001      // Protection Enable
  # It enables only segment-level protection. paging is not set by default
# There is no mode bit to disable segmentation. The use of paging, however is optional.(See Intel SDM vol3 Ch3.1)
# Note) If paging is not used, the linear address space of the processor is mapped directly into the physical address space of processor
  orl     $CR0_PE, %eax
  movl    %eax, %cr0

//PAGEBREAK!
  # Complete the transition to 32-bit protected mode by using a long jmp
  # to reload %cs and %eip.  The segment descriptors are set up with no
  # translation, so that the mapping is still the identity mapping.

  #define SEG_KCODE 1  // kernel code
  # cannot modify %cs directly, so instead the code executes an ljmp instruction
# set %cs <- $(SEG_CODE<<3) = 0x08, which is at the 2nd GDT entry
# To implement a basic flat memory model with the IA-32 architecture, at least two segment descriptors must be created, one for referencing a code segment and one for referencing a data segment. See Intel SDM vol3 3.2.1 Basic Flat Model.
# To implement a flat memory model without paging, software initialization code must at a minimum load a GDT with one code and one data-segment descriptor. A null descriptor in the first GDT entry is also required. The stack can be placed in a normal read/write data segment, so no dedicated descriptor for the stack is required. A flat memory model with paging also requires a page directory and at least one page table(See Intel SDM vol3 ch9.8.1 Protected-Mode System Data Structures)
  # %cs bit3~15: index, bit2: table indicator flag(0=GDT), RPL(Requested Priviledge Level):0(most privileged level)
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

  # For more information, see http://bochs.sourceforge.net/doc/docbook/development/debugger-advanced.html
  # If bootmain returns (it shouldn't), trigger a Bochs
  # Port 0x8A00 servers as command register. You can use it to enable the i/o interface, change which data register is active, etc.
  # Bochs is a highly portable open source IA-32 (x86) PC emulator
  # breakpoint if running under Bochs, then loop.
  # Used to enable the device. Any I/O to the debug module before this command is sent is sent will simply be ignored.
  movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
  movw    %ax, %dx
  outw    %ax, %dx
  # return the Bochs to the debugger prompt. Basically the same as doing CTRL+C.
  movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
  outw    %ax, %dx
spin:
  jmp     spin

# Bootstrap GDT
# Basic Flat model: hide the segmentation mechanism of the architecture  from both the system designer and the application programmer.
.p2align 2                                # force 4 byte alignment
gdt:
  # each entry is 8 byte
  SEG_NULLASM                             # null seg 0x0000 0000 00 00 00 00
  // #define SEG_ASM(type,base,lim)
  // segment base address is set to be 0x00
  SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
  SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg

gdtdesc:
  # The lower 16 bits tell the size of the GDT
  .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
  # the upper 32 bits tell the location of the GDT in memory
  .long   gdt                             # address gdt
