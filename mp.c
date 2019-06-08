// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mp.h"
#include "x86.h"
#include "mmu.h"
#include "proc.h"

struct cpu cpus[NCPU];
int ncpu;
// Assume that there is only one I/O APIC.
uchar ioapicid;

static uchar
sum(uchar *addr, int len)
{
  int i, sum;

  sum = 0;
  for(i=0; i<len; i++)
    sum += addr[i];
  return sum;
}

// Look for an MP structure in the len bytes at addr.
static struct mp*
mpsearch1(uint a, int len)
{
  uchar *e, *p, *addr;

  addr = P2V(a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(struct mp))
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
      return (struct mp*)p;
  return 0;
}

// Search for the MP Floating Pointer Structure, which according to the
// spec is in one of the following three locations:
// 1) in the first KB of the EBDA;
// 2) in the last KB of system base memory;
// 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
static struct mp*
mpsearch(void)
{
  uchar *bda;
  uint p;
  struct mp *mp;

  bda = (uchar *) P2V(0x400);
  // 1) in the first KB of the EBDA;
  // BIOS Data Area (BDA) starting at 0x0400
  // 0x40E .. EBDA base address >> 4 (usually!)
  if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
    if((mp = mpsearch1(p, 1024)))
      return mp;
  } else {
    // Memory size in Kbytes
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
    if((mp = mpsearch1(p-1024, 1024)))
      return mp;
  }
  return mpsearch1(0xF0000, 0x10000);
}

// Search for an MP configuration table.  For now,
// don't accept the default configurations (physaddr == 0).
// Check for correct signature, calculate the checksum and,
// if correct, check the version.
// To do: check extended table checksum.
static struct mpconf*
mpconfig(struct mp **pmp)
{
  struct mpconf *conf;
  struct mp *mp;

  if((mp = mpsearch()) == 0 || mp->physaddr == 0)
    return 0;
  conf = (struct mpconf*) P2V((uint) mp->physaddr);
  // multiple check
  if(memcmp(conf, "PCMP", 4) != 0)
    return 0;
  if(conf->version != 1 && conf->version != 4)
    return 0;
  if(sum((uchar*)conf, conf->length) != 0)
    return 0;
  *pmp = mp;
  return conf;
}
struct mp *gmp;

void
mpinit(void)
{
  uchar *p, *e;
  int ismp;
  struct mp *mp;
  struct mpconf *conf;
  struct mpproc *proc;
  struct mpioapic *ioapic;

  if((conf = mpconfig(&mp)) == 0)
    panic("Expect to run on an SMP");
  gmp = mp;
  ismp = 1;
  // [global] The base address by which each processor accesses its local APIC.
  lapic = (uint*)conf->lapicaddr;
  // find Processor or I/O APIC
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
    switch(*p){
    // TODO: Does this contain self cpu information? -> YES!
    case MPPROC:
      proc = (struct mpproc*)p;
      if(ncpu < NCPU) {
        // [global]
        /*
          struct cpu {
          uchar apicid;                // Local APIC ID
          struct context *scheduler;   // swtch() here to enter scheduler
          struct taskstate ts;         // Used by x86 to find stack for interrupt
          // #define NSEGS     6
          struct segdesc gdt[NSEGS];   // x86 global descriptor table
          volatile uint started;       // Has the CPU started?
          int ncli;                    // Depth of pushcli nesting.
          int intena;                  // Were interrupts enabled before pushcli?
          struct proc *proc;           // The process running on this cpu or null
        };
         */
        cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
        ncpu++;
      }
      p += sizeof(struct mpproc);
      continue;
    case MPIOAPIC:
      /*
        struct mpioapic {       // I/O APIC table entry
          uchar type;                   // entry type (2)
          uchar apicno;                 // I/O APIC id
          uchar version;                // I/O APIC version
          uchar flags;                  // I/O APIC flags
          uint *addr;                  // I/O APIC address
        };
       */
      ioapic = (struct mpioapic*)p;
      // [global] uchar ioapicid;
      ioapicid = ioapic->apicno;
      p += sizeof(struct mpioapic);
      continue;
    case MPBUS:
    case MPIOINTR:
    case MPLINTR:
      // ignore
      p += 8;
      continue;
    default:
      ismp = 0;
      break;
    }
  }
  if(!ismp)
    panic("Didn't find a suitable machine");

  // IMCR: interrupt mode configuration register: corporate with 8259A-EQUIVALENT PICS
  // IMCRP. When the IMCR presence bit is set, the IMCR is present and PIC Mode is implemented; otherwise, Virtual Wire Mode is implemented.
  // When the operating system is ready to switch to MP operation, it writes a 01H to the IMCR register, if that register is implemented, and enables I/O APIC Redirection Table entries.
  if(mp->imcrp){
    // Bochs doesn't support IMCR, so this doesn't run on Bochs.
    // But it would on real hardware.
    outb(0x22, 0x70);   // Select IMCR
    // write imcr's data register
    outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
  }
}
