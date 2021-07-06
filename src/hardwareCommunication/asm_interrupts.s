.set IRQ_BASE, 0x20
.section .text
.extern __ZN4zoeos21hardwareCommunication16InterruptManager15HandleInterruptEhj

.macro HandleInterruptRequest num
.global __ZN5zoeos21hardwareCommunication16InterruptManager26HandleInterruptRequest\num\()Ev
__ZN5zoeos21hardwareCommunication16InterruptManager26HandleInterruptRequest\num\()Ev:
    movb $\num + IRQ_BASE, (interruptnumber)
    pushl $0
    jmp int_bottom
.endm

.macro HandleException num
.global __ZN5zoeos21hardwareCommunication16InterruptManager19HandleException\num\()Ev
__ZN5zoeos21hardwareCommunication16InterruptManager19HandleException\num\()Ev:
    movb $\num, (interruptnumber)
    jmp int_bottom
.endm

HandleInterruptRequest 0x00
HandleInterruptRequest 0x01
HandleInterruptRequest 0x02
HandleInterruptRequest 0x03
HandleInterruptRequest 0x04
HandleInterruptRequest 0x05
HandleInterruptRequest 0x06
HandleInterruptRequest 0x07
HandleInterruptRequest 0x08
HandleInterruptRequest 0x09
HandleInterruptRequest 0x0A
HandleInterruptRequest 0x0B
HandleInterruptRequest 0x0C
HandleInterruptRequest 0x0D
HandleInterruptRequest 0x0E
HandleInterruptRequest 0x0F
HandleInterruptRequest 0x31

HandleException 0x00
HandleException 0x01
HandleException 0x02
HandleException 0x03
HandleException 0x04
HandleException 0x05
HandleException 0x06
HandleException 0x07
HandleException 0x08
HandleException 0x09
HandleException 0x0A
HandleException 0x0B
HandleException 0x0C
HandleException 0x0D
HandleException 0x0E
HandleException 0x0F
HandleException 0x10
HandleException 0x11
HandleException 0x12
HandleException 0x13

int_bottom:
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    pushl %esp

    push (interruptnumber)
    call __ZN5zoeos21hardwareCommunication16InterruptManager15handleInterruptEhj

    movl %eax, %esp

    popl %eax
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp

    add $4, %esp

; .global __ZN16InterruptManager15interruptIgnoreEv
;     __ZN16InterruptManager15interruptIgnoreEv:

    iret

.data
    interruptnumber: .byte 0

