# Initial process execs /init.
# This code runs in user space.

#include "syscall.h"
#include "traps.h"


# exec(init, argv)
.globl start
start:
  pushl $argv
  pushl $init
  pushl $0  // where caller pc would be
  movl $SYS_exec, %eax
  // [hardware]
  // 1. fetch the n'th descriptor from the IDT, where n is the arguement of `int`
  // 2. check that CPL in %cs(in IDT) is DPL
  // 3. save %esp and %ss in CPU-internal registers only if PL < CPL
  // 4. load %ss and %sep from a task segment desc
  // 5. push %ps, %esp, %eflags, %cs, %eip
  // 6. clear the IF(Interrupt enable flag) bit in %eflags
  // 7. set %cs and %eip to the values in the TSS. %eip register tells the computer where to go next to execute the next command and controls the flow of a program.

  // [software]
  // 8. call vector60
  // 9. pushl $0, which is errcode
  // 10. push $60, which is trapno
  // 11. jmp alltraps
  // 12. push %ds, %es, %fs, %gs
  // 13. push general purpose registers by `pushal`
  // 14. call trap()
  // 14-1. call syscall()
  // 14.2. Now that `num = curproc->tf->eax` is equal to `$SYS_exec`, and the function associated with $SYS_exec number, sys_exec(void) is called.
  // 15. pop general purpose registers
  // 16. pop %ds, %es, %fs, %gs
  // 17. pop errcode and trapno
  // 18. call iret, which jumps back into user space -> [hardware]
  int $T_SYSCALL

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

