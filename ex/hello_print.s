.text
  .globl main

main:
  movl $exit, %eax
  pushl %eax
  jmp hello

hello:
  movl $0x04, %eax
  movl $0x01, %ebx
  movl $message, %ecx
  movl $13, %edx # length
  int $0x80
  ret

exit:
  # exit
  movl $0x01, %eax
  xorl %ebx, %ebx
  int $0x80
  
message:
  .asciz "Hello world\n"
