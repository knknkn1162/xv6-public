// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
//
#define PGSIZE 4096
#define NPDENTRIES 1024
#define KERNBASE 0x80000000
#define PDXSHIFT        22      // offset of PDX in a linear address
// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PS          0x080   // Page Size
typedef unsigned int pde_t;


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

int
main(void)
{
  for(;;)
    ;
}
