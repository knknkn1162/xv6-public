  .text
  .globl main

main:
  pushl $message
  pushl $size
  call hello
  addl $0x08, %esp
  movl $0x01, %eax
  xorl %ebx, %ebx
  int $0x80
hello:
  // save %ebp
  pushl %ebp
  // save current %esp address in %ebp as temporary value, because real %esp may change.
  movl %esp, %ebp
  movl 8(%ebp), %edx
  movl (%edx), %edx
  movl 12(%ebp), %ecx
  movl $0x04, %eax
  movl $0x01, %ebx
  int $0x80
  movl %ebp, %esp
  // restore %ebp
  popl %ebp
  ret

message:
  .asciz "Hello world!\n"
size:
  .long   (size - message - 1)
