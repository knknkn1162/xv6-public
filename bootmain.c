// Boot loader.
//
// Part of the boot block, along with bootasm.S, which calls bootmain().
// bootasm.S has put the processor into protected 32-bit mode.

#include "types.h"
#include "elf.h"
#include "x86.h"
#include "memlayout.h"

#define SECTSIZE  512

void readseg(uchar*, uint, uint);

// bootmain() loads an ELF kernel image from the disk starting at
// sector 1 and then jumps to the kernel entry routine.
void
bootmain(void)
{
  struct elfhdr *elf;
  struct proghdr *ph, *eph;
  void (*entry)(void);
  uchar* pa;

  // ELF binary places in-memory copy at address 0x10000
  elf = (struct elfhdr*)0x10000;  // scratch space

  // Read 1st page off disk
  readseg((uchar*)elf, 4096, 0); // 4096 = 0x1000

  // Is this an ELF executable?
  if(elf->magic != ELF_MAGIC)
    return;  // let bootasm.S handle error

  // Load each program segment (ignores ph flags).
  // program header table offset, 0 if not present.
  ph = (struct proghdr*)((uchar*)elf + elf->phoff);
  // number of program header entries, 0 if not present.
  eph = ph + elf->phnum;
  for(; ph < eph; ph++){
    pa = (uchar*)ph->paddr;
    // read into physical memory
    // TODO: what is ph->off?
    readseg(pa, ph->filesz, ph->off);
    // If the segment’s memory size (p_memsz) is larger than the file size (p_filesz), the ‘‘extra’’ bytes are defined to hold the value 0 and to follow the segment’s initialized area.
    if(ph->memsz > ph->filesz)
      // stosb(void *addr, int data, int cnt)
      // initialize every byte of a block of memory
      stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
  }

  // Call the entry point from the ELF header.
  // Does not return!
  entry = (void(*)(void))(elf->entry); // address is 0x10000c
  entry();
}

void
waitdisk(void)
{
  // Wait for disk ready.
  // send command to port 0x1F7
  while((inb(0x1F7) & 0xC0) != 0x40)
    ;
}

// Read a single sector at offset into dst.
void
readsect(void *dst, uint offset)
{
  // Issue command.
  waitdisk();
  //   asm volatile("out %0,%1" : : "a" (data), "d" (port));
  // Number of sectors to read/write (0 is a special value).
  // The kernel has been written to the boot disk contiguously starting at sector 1.
  outb(0x1F2, 1);   // count = 1
  // This is CHS / LBA28 / LBA48 specific.
  outb(0x1F3, offset);
  // 	Partial Disk Sector address.(0)
  outb(0x1F4, offset >> 8);
  outb(0x1F5, offset >> 16);
  // Send 0xE0 for the "master"
  outb(0x1F6, (offset >> 24) | 0xE0);
  outb(0x1F7, 0x20);  // cmd 0x20 - read sectors

  // Read data.
  waitdisk();
  // input doubleword * SECTSIZE/4 bytes from 0x1F0 into dst
  insl(0x1F0, dst, SECTSIZE/4);
}

// Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
// Might copy more than asked.
void
readseg(uchar* pa, uint count, uint offset)
{
  uchar* epa;

  epa = pa + count;

  // Round down to sector boundary.
  pa -= offset % SECTSIZE;

  // Translate from bytes to sectors; kernel starts at sector 1.
  offset = (offset / SECTSIZE) + 1;

  // If this is too slow, we could read lots of sectors at a time.
  // We'd write more to memory than asked, but it doesn't matter --
  // we load in increasing order.
  for(; pa < epa; pa += SECTSIZE, offset++)
    readsect(pa, offset);
}
