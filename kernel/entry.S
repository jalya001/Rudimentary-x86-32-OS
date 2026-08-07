.section .rodata
si_msg:
  .asciz "Syscall"
s2_msg:
  .asciz "FINISH SYSCALL"
tim_msg:
  .asciz "Timer Interrupt"

.section .text

.extern fault_print

.globl enter_critical
.globl leave_critical
.globl scheduler_entry
.globl syscall_entry
.globl timer_handler

.extern syscall_handler

enter_critical:
  cli
  ret

leave_critical:
  sti
  ret

timer_handler: # does nothing for now
  pusha

  push $0
  push $tim_msg
  call fault_print
  add $8, %esp

  mov $0x20, %al
  out %al, $0x20

  popa
  iret

scheduler_entry:
  ret

syscall_entry:
  pushal

  # args
  pushl	%edx
  pushl	%ecx
  pushl	%ebx
  pushl	%eax

  push %eax
  push $si_msg
  call fault_print
  add $8, %esp
  
  call syscall_handler
  add $16, %esp # pop args

  popal

  iret