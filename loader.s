# initalize os
# https://blog.csdn.net/weixin_34357887/article/details/91741929

# global definitions
# bootloader entrance address
.set MAGIC, 0x1badb002
.set FLAGS, (1 << 0 | 1 << 1)
.set CHECKSUM, -(MAGIC + FLAGS)

# use grub to load os
.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

# text segment
.section .text
# the statement of external functions
.extern __Z16callConstructorsv
.extern __Z10kernelMainPvj
# expose a symbol to ld and other modules
.global loader

loader:
    mov $kernel_stack, %esp
    call __Z16callConstructorsv
    push %eax       # bootloader's address in %eax
    push %ebx       # magic number in %ebx
    call __Z10kernelMainPvj

stop:
    # "cli" clear all interruptions
    # after hlt there is no way to wake up cpu again
    cli
    # real mode or ring 0 privilege class
    # pause cpu
    hlt
    jmp stop

.section .bss
.space 2 * 1024 * 1024
kernel_stack:
