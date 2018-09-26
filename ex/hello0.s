.text
  .globl main

main:
  # sys_write(0x04)の呼び出し
  movl $0x04, %eax
  movl $0x01, %ebx
  movl $message, %ecx
  movl $12, %edx # length
  int $0x80
  # sys_exit(0x01)の呼び出し
  movl $0x01, %eax
  xorl %ebx, %ebx
  int $0x80

message:
  .ascii "Hello world\n"
