#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

static void startothers(void);
static void mpmain(void)  __attribute__((noreturn));
extern pde_t *kpgdir;

/* 0x801020c0                entrypgdir */
/* [!provide]                PROVIDE (end = .) */
extern char end[]; // first address after kernel loaded from ELF file

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
//
// At this section, The address means the virtual memory address
int
main(void)
{
  // 4MB free
  // for much of main, one cannot use locks or memory about 4MB=(0x100000)
  // using entrypgdir to place just the pages
  // free address at end=ceil(x801020c0)(=0x80103000) ~ 0x80503000
  // At this stage, pgdir is defined at 0x80000000~0x80400000,
  kinit1(end, P2V(4*1024*1024)); // phys page allocator

  kvmalloc();      // kernel page table
  mpinit();        // detect other processors
  /* ignores interrupts from the PIC, and configures the IOAPIC and local APIC */
  // initialize local APIC
  lapicinit();     // interrupt controller
  // initiazlie global descriptor table(GDT)
  // struct segdesc gdt[NSEGS];   // x86 global descriptor table
  seginit();       // segment descriptors
  // Don't use the 8259A interrupt controllers.  Xv6 assumes SMP hardware.
  picinit();       // disable pic
  ioapicinit();    // another interrupt controller

  consoleinit();   // console hardware
  uartinit();      // serial port
  // lock ptable
  pinit();         // process table

  // setup the 256 entries in the table IDT(Interrupt descriptor table)
  // set interrupt gate & trap gate
  tvinit();        // trap vectors
  //   // Create linked list of buffer cache
  binit();         // buffer cache
  // only filetable lock
  fileinit();      // file table
  // initialize the Disk Driver(IDE: Integrated Device Electronics)
  // Check if disk 1 is present
  // polls the status bits until the busy bit is clear and the ready byit is set
  ideinit();       // disk 
  startothers();   // start other processors
  // enable locking and arrange for more memory to be allocate
  // release reminder memory
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
  userinit();      // first user process
  mpmain();        // finish this processor's setup
}

// Other CPUs jump here from entryother.S.
static void
mpenter(void)
{
  // set CR3 register
  switchkvm();
  // Set up CPU's kernel & user's segment descriptors.
  seginit();
  // set local APIC
  lapicinit();
  mpmain();
}

// Common CPU setup code.
static void
mpmain(void)
{
  cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
  // vector 32(=IRQ 0) is an interrupt gate
  idtinit();       // load idt register
  // when the BSP(bootstrap processor) and the application processor(AP) are initialized, the BSP then begins executing the OS initialization code.
  xchg(&(mycpu()->started), 1); // tell startothers() we're up
  // scheduler enables interrupts
  scheduler();     // start running processes
}

pde_t entrypgdir[];  // For entry.S

// Start the non-boot (AP) processors.
static void
startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_size[];
  uchar *code;
  struct cpu *c;
  char *stack;

  // Write entry code to unused memory at 0x7000.
  // 0x00000500 ~ 0x00007BFF: guaranteed free for use
  // The linker has placed the image of entryother.S in
  // _binary_entryother_start.
  code = P2V(0x7000);
  /* try `objcopy -I binary -O elf32-i386 -B i386 entryother.o outfile && readelf -s outfile`
       -B bfdarch
           Useful when transforming a architecture-less input file into an object file.  In this case the output architecture can be set to bfdarch.  This option will be ignored if the input file has a known bfdarch.  You can access this binary data inside a program by referencing the special symbols that are created by the conversion process.  These symbols are called _binary_objfile_start, _binary_objfile_end and _binary_objfile_size.  e.g. you can transform a picture file into an object file and then access it in your code using these symbols.
   */
  /* move to physical memory */
  memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);

  for(c = cpus; c < cpus+ncpu; c++){
    if(c == mycpu())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kalloc();
    *(void**)(code-4) = stack + KSTACKSIZE;
    *(void(**)(void))(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    // Start additional processor running entry code at addr.
    lapicstartap(c->apicid, V2P(code));

    // wait for cpu to finish mpmain():86
    while(c->started == 0)
      ;
  }
}

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  // (0) | 0x083
  // PTE_P: whether PTE is present
  // PTE_W: instructions are allowed to issue writes to the page
  // PTE_PS: enables 4Mbyte tables(when CR4.PSE is set)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  // page directory index
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

