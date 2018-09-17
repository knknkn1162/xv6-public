// The I/O APIC manages hardware interrupts for an SMP system.
// http://www.intel.com/design/chipsets/datashts/29056601.pdf
// See also picirq.c.

#include "types.h"
#include "defs.h"
#include "traps.h"

#define IOAPIC  0xFEC00000   // Default physical address of IO APIC

// see 3.0 Table2 in https://pdos.csail.mit.edu/6.828/2016/readings/ia32/ioapic.pdf
#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)

// The number of I/O APIC assumes to be 1
volatile struct ioapic *ioapic;

// IO APIC MMIO structure: write reg, then read or write data.
// The IOREGSEL and IOWIN Registers (Table 3.1) can be relocated via the APIC Base Address Relocation Register in the PIIX3 and are aligned on 128 bit boundaries.
// based on REG_TABLE (0x10)
struct ioapic {
  uint reg;
  uint pad[3];
  uint data;
};

static uint
ioapicread(int reg)
{
  ioapic->reg = reg;
  return ioapic->data;
}

static void
ioapicwrite(int reg, uint data)
{
  ioapic->reg = reg;
  ioapic->data = data;
}

void
ioapicinit(void)
{
  int i, id, maxintr;

  // #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
  ioapic = (volatile struct ioapic*)IOAPIC;
  // #define REG_VER    0x01  // Register index: version
  maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
  // #define REG_ID     0x00  // Register index: ID
  id = ioapicread(REG_ID) >> 24;
  if(id != ioapicid)
    cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");

  // Mark all interrupts edge-triggered, active high, disabled,
  // and not routed to any CPUs.
  // #define REG_TABLE  0x10  // Redirection table base
  // This behavior is identical to the case where the device withdraws the interrupt before that interrupt is posted to the processor. **It is software's responsibility to handle the case where the mask bit is set** after the interrupt message has been accepted by a local APIC unit but before the interrupt is dispensed to the processor.
  // xv6 programs to map interrupt 0 to IRQ 0, and so on, but disables them all. Specific devices enable particular interrupts and say to which processor the interrupt should be routed. For example, xv6 routes keyboard interrupts to processor 0 (8274). Xv6 routes disk interrupts to the highest numbered processor on the system..
  for(i = 0; i <= maxintr; i++){
    // See IOAPIC Specification ch.3.2.4. IOREDTBL
    // #define INT_DISABLED   0x00010000  // Interrupt disabled
    // #define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ
    ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
    ioapicwrite(REG_TABLE+2*i+1, 0);
  }
}

void
ioapicenable(int irq, int cpunum)
{
  // Mark interrupt edge-triggered, active high,
  // enabled, and routed to the given cpunum,
  // which happens to be that cpu's APIC ID.
  // #define REG_TABLE  0x10  // Redirection table base
  ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
  ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
}
