// See MultiProcessor Specification Version 1.[14]
// https://pdos.csail.mit.edu/6.828/2018/readings/ia32/MPspec.pdf

struct mp {             // floating pointer
  // The ASCII string represented by “_MP_” which serves as a search key for locating the pointer structure.
  uchar signature[4];           // "_MP_"
  // The address of the beginning of the MP configuration table. All zeros if the MP configuration table does not exist.
  void *physaddr;               // phys addr of MP config table
// The length of the floating pointer structure table in paragraph (16-byte) units. The structure is 16 bytes or 1 paragraph long; so this field contains 01h.
  uchar length;                 // 1
// The version number of the MP specification supported. A value of 01h indicates Version 1.1. A value of 04h indicates Version 1.4.
  uchar specrev;                // [14]
  //  A checksum of the complete pointer structure. All bytes specified by the length field, including CHECKSUM and reserved bytes, must add up to zero.
  uchar checksum;               // all bytes must add up to 0
  uchar type;                   // MP system config type
  // Bits 0-7: MP System Configuration Type. When these bits are all zeros, the MP configuration table is present.
  uchar imcrp;
  uchar reserved[3];
};

struct mpconf {         // configuration table header
  uchar signature[4];           // "PCMP"
  ushort length;                // total table length
  uchar version;                // [14]
  uchar checksum;               // all bytes must add up to 0
  uchar product[20];            // product id
  uint *oemtable;               // OEM table pointer
  ushort oemlength;             // OEM table length
  ushort entry;                 // entry count
  // The base address by which each processor accesses its local APIC.
  uint *lapicaddr;              // address of local APIC
  ushort xlength;               // extended table length
  uchar xchecksum;              // extended table checksum
  uchar reserved;
};

struct mpproc {         // processor table entry
  uchar type;                   // entry type (0)
  uchar apicid;                 // local APIC id
  uchar version;                // local APIC verison
  uchar flags;                  // CPU flags
    #define MPBOOT 0x02           // This proc is the bootstrap processor.
  uchar signature[4];           // CPU signature
  uint feature;                 // feature flags from CPUID instruction
  uchar reserved[8];
};

struct mpioapic {       // I/O APIC table entry
  uchar type;                   // entry type (2)
  uchar apicno;                 // I/O APIC id
  uchar version;                // I/O APIC version
  uchar flags;                  // I/O APIC flags
  uint *addr;                  // I/O APIC address
};

// Table entry types
// See table 4.3 in https://pdos.csail.mit.edu/6.828/2018/readings/ia32/MPspec.pdf
#define MPPROC    0x00  // One per processor
#define MPBUS     0x01  // One per bus
#define MPIOAPIC  0x02  // One per I/O APIC
#define MPIOINTR  0x03  // One per bus interrupt source
#define MPLINTR   0x04  // One per system interrupt source

//PAGEBREAK!
// Blank page.
