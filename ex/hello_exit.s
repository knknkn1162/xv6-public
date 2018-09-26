.text
  .globl main

main:
  call hello
  call exit

hello:
  movl $0x04, %eax
  movl $0x01, %ebx
  movl $message, %ecx
  movl $12, %edx # length
  int $0x80
  ret

exit:
  # exit
  movl $0x01, %eax
  xorl %ebx, %ebx
  int $0x80
  
message:
  .ascii "Hello world\n"
