.section .rodata
di_msg:
	.asciz "Default interrupt"
gp_msg:
	.asciz "General Protection Fault code"
df_msg:
	.asciz "Double Fault at EIP"

pf_err_msg:
  .asciz "Page Fault error"
pf_addr_msg:
  .asciz "Page Fault at CR2"

.section .text

.extern fault_print

.global general_protection_handler
.global double_fault_handler
.global default_interrupt_handler
.global page_fault_handler

default_interrupt_handler:
	pusha

	push $0
	push $di_msg
	call fault_print
	add $8, %esp

	# EOI
	mov $0x20, %al
	out %al, $0x20

	popa
	iret

general_protection_handler:
	pusha

	mov 32(%esp), %eax

	push %eax
	push $gp_msg
	call fault_print
	add $8, %esp

	popa

	cli
1:
	hlt
	jmp 1b


double_fault_handler:
	pusha

	mov 32(%esp), %eax

	push %eax
	push $df_msg
	call fault_print
	add $8, %esp

	popa

	cli
1:
	hlt
	jmp 1b

page_fault_handler:
	pusha

	mov 32(%esp), %eax
	push %eax
	push $pf_err_msg
	call fault_print
	add $8, %esp

	mov %cr2, %eax
	push %eax
	push $pf_addr_msg
	call fault_print
	add $8, %esp

	popa

	cli
1:
	hlt
	jmp 1b