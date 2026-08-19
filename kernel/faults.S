.section .rodata
di_msg:
	.asciz "Default interrupt"
gp_msg:
	.asciz "General Protection Fault code"
df_msg:
	.asciz "Double Fault at EIP"
de_msg:  .asciz "Divide Error"
db_msg:  .asciz "Debug"
nmi_msg: .asciz "NMI"
bp_msg:  .asciz "Breakpoint"
of_msg:  .asciz "Overflow"
br_msg:  .asciz "Bounds"
ud_msg:  .asciz "Invalid Opcode"
nm_msg:  .asciz "Device Not Available"
co_msg:  .asciz "Coprocessor Segment Overrun"
ts_msg:  .asciz "Invalid TSS"
np_msg:  .asciz "Segment Not Present"
ss_msg:  .asciz "Stack Segment Fault"
pf_msg:  .asciz "Page Fault"
mf_msg:  .asciz "x87 FPU Error"
ac_msg:  .asciz "Alignment Check"
mc_msg:  .asciz "Machine Check"
xm_msg:  .asciz "SIMD FPU Error"


.section .text

.extern fault_print

.globl default_interrupt_handler
.globl divide_error_handler
.globl debug_handler
.globl nmi_handler
.globl breakpoint_handler
.globl overflow_handler
.globl bounds_handler
.globl invalid_opcode_handler
.globl device_not_available_handler
.globl double_fault_handler
.globl coprocessor_segment_handler
.globl invalid_tss_handler
.globl segment_not_present_handler
.globl stack_segment_handler
.globl general_protection_handler
.globl page_fault_handler
.globl x87_fpu_handler
.globl alignment_check_handler
.globl machine_check_handler
.globl simd_fpu_handler

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


divide_error_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $de_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

debug_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $db_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

nmi_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $nmi_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

breakpoint_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $bp_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

overflow_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $of_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

bounds_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $br_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

invalid_opcode_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $ud_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

device_not_available_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $nm_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

coprocessor_segment_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $co_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

invalid_tss_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $ts_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

segment_not_present_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $np_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

stack_segment_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $ss_msg
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
	push $pf_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

x87_fpu_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $mf_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

alignment_check_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $ac_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

machine_check_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $mc_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b

simd_fpu_handler:
	pusha
	mov 32(%esp), %eax
	push %eax
	push $xm_msg
	call fault_print
	add $8, %esp
	popa
	cli
1:
	hlt
	jmp 1b