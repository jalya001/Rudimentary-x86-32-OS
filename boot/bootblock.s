.text
# .globl _start
.code16

.equ BOOT_SEGMENT,0x07c0
# .equ DISPLAY_SEGMENT,0xb800

_start:
	jmp	over
os_size:
	.long 0
over:
	# 1. Set up stack
	movw $0x7000, %ax
	movw %ax, %ss
	movw $0xfffe, %sp

	# 2. Set up data segments
	movw $BOOT_SEGMENT, %ax
	movw %ax, %ds
	movw $BOOT_SEGMENT,%ax
	movw %ax,%es

	# 3. Load kernel
	movw $0x0800, %ax
	movw %ax, %es
	xorw %bx, %bx

	movb $0, %ch
	movb $2, %cl
	movb $0, %dh
	movb $0x80, %dl

	movl os_size, %eax
	movb %al, %al # We are only using 8 bits for now, rather than reading in a loop
	movb $2, %ah
	int $0x13

	# 4. Enable A20
	# Uses fast A20. Should have legacy version and a check if A20 already is enabled for robustness 
	inb $0x92, %al
	orb $0x02, %al
	outb %al, $0x92

	# 5. Set GDT
	cli
	lgdt gdtr

	# 6. Set CR0's Protected Mode bit (legacy method)
	smsw %ax
	orw $1, %ax
	lmsw %ax

	# 7. Long jump to kernel
	ljmp $0x08, $next + (BOOT_SEGMENT << 4)

.code32
next:
	movw $0x10, %ax

	movw %ax, %ss
	movl $0x80000, %esp

	movw %ax, %ds
	movw %ax, %es

	jmp kernel_jump

.code16
forever:
	jmp	forever


gdtr:
	.word 23 # 24 bytes of entries counting from 0
	.long gdt + (BOOT_SEGMENT << 4)

gdt:
	# Null entry
	.long  0
	.long  0

	# Code entry
	.word 0xffff
	.word 0x0000
	.byte 0x00
	.word 0xc09a
	.byte 0x00

	# Data entry
	.word 0xffff
	.word 0x0000
	.byte 0x00
	.word 0xc092
	.byte 0x00


.code32
kernel_jump:
	.org 502
	ljmp $0x08, $0x0

	.org 510
	.word 0xAA55
