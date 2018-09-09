.text
  .globl main

main:
  movl $0x04, %eax
  movl $0x01, %ebx
  movl $message, %ecx
  movl $12, %edx # length
  int $0x80
  # jmp exit
  ljmp %cs:exit

exit:
  movl $0x01, %eax
  xorl %ebx, %ebx
  int $0x80
  
.section .rodata
message:
  .ascii "Hello world\n"
