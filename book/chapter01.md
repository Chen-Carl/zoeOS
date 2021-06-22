# 使用C/C++实现一个操作系统

# chapter 01 Hello World

## 1.1 操作系统的启动流程

**GNU GRUB(GRand Unified Bootloader, GRUB)** 是一个来自GNU项目的多操作系统启动程序。GRUB是多启动规范的实现，它允许用户可以在计算机内同时拥有多个操作系统，并在计算机启动时选择希望运行的操作系统。

使用GRUB引导操作系统的过程为：

1. BIOS转向第1块硬盘的第1个扇区，即 **主引导记录(MBR)**；

    BIOS系统存储于主板的ROM芯片上，计算机开机时，先读取该系统，然后进行加电自检。这个过程中检查CPU和内存、计算机基本的组成单元(控制器、运算器和存储器)，以及检查其他硬件，若没有异常就开始加载BIOS程序到内存当中。

2. MBR中存储了BootLoader信息，BootLoader将加载GRUB；

    MBR的工作是查找并加载第二段BootLoader程序。但在系统没启动时，MBR无法识别文件系统，因此需要在这一步加载GRUB。

3. GRUB查找并加载kernel；

    GRUB识别文件系统，根据/boot/grub/grub.conf文件查找Kernel信息并加载Kernel程序。当Kernel程序被检测并加载到内存中，GRUB就将控制权交接给Kernel程序。

4. kernel装载驱动，挂载ROOTFS，执行/sbin/init;

5. init初始化os，执行runlevel相关程序。

## 1.2 使用Multiboot规范编写loader.s

BootLoader是一段汇编代码，文件名为loader.s，它需要按照Multiboot规范来编译内核才可以被GRUB引导。

按照Mutileboot规范，内核必须在起始的8KB中的包含一个**多引导项头(Multiboot header)**，里面必须包含3个4字节对齐的块：
- 一个魔数块：包含了魔数`0x1BADB002`，表示这是多引导项头结构的定义
- 一个标志块：指出OS映像需要引导程序提供或支持的特性
- 一个校检块：校验块与魔数块和标志块相加的总和应为0

``` s
# os/loader.s
# 多引导项头
.set MAGIC, 0x1badb002;                 # 魔数块
.set FLAGS, (1<<0 | 1<<1);              # 标志块
.set CHECKSUM, -(MAGIC + FLAGS);        # 校验块

# 下面的伪指令声明了Multiboot标准中的多引导项头
# 三个块都是32位字段
.section .multiboot
    .long MAGIC
    .long FLAGS
    .long CHECKSUM
```

在多引导项头之后，是程序的入口点。kernel的代码在文件kernel.cpp中，loader.s在入口点中跳转到kernel.cpp的函数中执行。首先需要使用`.global`伪指令告诉链接器程序的入口点，用`loader`表示，最后是`stop`代码。

kernel.cpp中主函数的名称是`kernelMain`，需要传入两个参数，分别是BootLoader的地址和魔数，它们存放于寄存器`%eax`和`%ebx`中，将它们压栈以传递参数。`kernelMain`是一个外部符号，需要提前在loader.s中声明，这些外部符号存在于`.text`段中。同样地，为了使得`loader`函数对外部可见，使用`.global`伪指令向外暴露`loader`。

``` s
# os/loader.s
.section .multiboot
    # ...
    .long CHECKSUM

.section .text
.extern kernelMain
.global loader

loader:
    push %eax       # bootloader's address in %eax
    push %ebx       # magic number in %ebx
    call kernelMain

stop:
    # 禁用中断
    cli
    # 禁用中断后使用hlt暂停CPU，以后无法再唤醒
    hlt
    jmp stop
```

## 1.3 编写kernel.cpp核心代码

在`kernelMain`函数中接收到两个参数并简单打印`hello world`信息在显示器上。
``` cpp
// os/kernel.cpp
void kernelMain(void *multiboot_structure, unsigned int magicnumber)
{
    printf("hello world");
    while (1);
}
```

需要注意的是这时候是找不到标准库的`printf`函数的，因此需要自己定义。为了将字符串打印到屏幕，需要知道显示器的地址，这个地址为`0xb8000`，地址是short类型的，打印字符串时，将需要打印的字符（8位ASCII码）写入显示器的低16位。
``` cpp
// os/kernel.cpp
void printf(const char *str)
{
    unsigned short *VideoMemory = (unsigned short*)0xb8000;
    for (int i = 0; str[i]; i++)
    {
        VideoMemory[i] = (VideoMemory[i] & 0xFF00) | str[i];
    }
}

void kernelMain(void *multiboot_structure, unsigned int magicnumber)
{
    // ...
}
```

编译时需要取消g++自动添加的链接库，编译参数如下：
``` makefile
-m32                    # 指定编译为32位应用程序
-fno-use-cxa-atexit     # 禁用C析构函数__cxa_atexit
-fleading-underscore    # 编译时在C语言符号前加上下划线
-fno-exceptions         # 禁用异常机制
-fno-builtin            # 不使用C语言的内建函数
-nostdlib               # 不链接系统标准启动文件和标准库文件
-fno-rtti               # 禁用运行时类型信息
-fno-pie                # 禁用PIE模式
```

编写Makefile文件如下：
``` makefile
GPPPARAMS = -m32 -fno-use-cxa-atexit -fleading-underscore -fno-exceptions -fno-builtin -nostdlib -fno-rtti -fno-pie
ASPARAMS = --32
LDPARAMS = -melf_i386 -no-pie

objects = loader.o kernel.o

%.o: %.cpp
	g++ ${GPPPARAMS} -o $@ -c $<

%.o: %.s
	as ${ASPARAMS} -o $@ $<
```

将kernel.cpp编译成.o文件后查看符号表可以发现，`kernelMain`在符号表中的名字是`__Z10kernelMainPvj`，于是修改loader.s文件的`loader`部分为
```
loader:
    push %eax       # bootloader's address in %eax
    push %ebx       # magic number in %ebx
    call __Z10kernelMainPvj
```

在调用`kernelMain`之前，还需要一个步骤保证初始化必要的全局变量，这些全局变量存放在`.init_array`段中。为了在`loader`中首先执行`.init_array`段初始化的步骤，我们添加一个初始化函数`callConstructors()`，该函数获取`.init_array`段的地址并执行段中的指令。
``` cpp
// os/kernel.cpp
void kernelMain(void *multiboot_structure, unsigned int magicnumber)
{
    // ...
}

typedef void (*constructor)();
constructor start_ctors;
constructor end_ctors;

void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
    {
        (*i)();
    }
}
```
现在还无法确定`start_ctors`和`end_ctors`的具体地址，需要告诉链接器由它来确定具体地址，然后将初始化指令放在起始地址和终止地址之间，保证在循环中两者之间的指令都能被执行。后面将使用linker脚本文件指定链接器的行为。
``` bash
> make kernel.o
> nm kernel.o
```
可以查看kernel.o的符号表，发现两个函数指针`start_ctors`和`end_ctors`在符号表中的记号为`_start_ctors`和`_end_ctors`，建立链接脚本文件linker.ld使用下面的指令保证执行`.init_array`段的部分：
``` cpp
/* os/linker.ld */
.data :
{
    _start_ctors = .;
    KEEP(*(.init_array));
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array)));
    _end_ctors = .;
}
```

在loader.s中需要在`loader`开始处调用这个函数，同样，我们需要查看符号表，将`callConstructors()`对应的符号添加到外部符号声明中：
``` s
# os/loader.s
# ...
.extern __Z16callConstructorsv
.extern __Z10kernelMainPvj
```

除此以外，还需要初始化内核栈，内核栈的地址存放于寄存器`%esp`中。根据上面所述，修改loader.s文件。
``` s
# os/loader.s
loader:
    mov $kernel_stack, %esp
    call __Z16callConstructorsv
    push %eax
    push %ebx
    call __Z10kernelMainPvj
```

在loader.s的最后需要使用伪指令`.space`为.bss段分配空间，这里分配的大小为2M。
``` s
stop:
    # ...
    jmp stop

.section .bss
.space 2 * 1024 * 1024
kernel_stack:
```

至此，已经基本完成了启动操作系统的主要部分。

## 1.4 linker脚本文件的编写

linker脚本文件是用来控制link过程的文件，文件中包含内容为linker的处理命令，主要用于描述输入文件到输出文件(目标文件)时各个内容的的分布及内存映射等等。在上一节中的linker.ld已经告诉了链接器把需要初始化的部分放在`_start_ctors`和`_end_ctors`之间，关于链接脚本格式还有一些其他内容需要指定。

linker脚本最简单的格式为
``` cpp
SECTIONS
{
    . = 0x10000;
    .text : { *(.text) }
    . = 0x8000000;
    .data : { *(.data) }
    .bss : { *(.bss) }
}
```

`.text`是输出节。在大括号里列出输入节的名字，它们会放入输出节中。使用通配符匹配任何文件名，表达式`*(.text)`意味着所有的输入文件中的输入节`.text`。`*`表示通配符，`.`是位置计数器，它以输出节的大小增加其值，链接器会设置输出文件中的`.text`节的地址为0x10000。

剩下的行定义输出文件中的`.data`和`.bss`节。链接器会把输出节`.data`放置到地址0x8000000。之后，链接器把输出节`.data`的大小加到位置计数器的值0x8000000，并立即设置`.bss`输出节，效果是在内存中，`.bss`节会紧随`.data`之后（两个节之间可能产生必要的对齐）。

对于一个被抛弃的的输出段，链接器将会忽略给该段指定的地址，除非在输出段中有符号定义，那样的话，即使这个输出段被抛弃，链接器依然会遵守该段地址的指定。一个特殊的输出段名`/DSICARD/`可以用来给抛弃输入段使用，任何被放在名字为`/DISCARD/`输出段中的输入段都不会被包含到输出文件中。

``` cpp
/* set the entry point */
ENTRY(loader)
/* name the format to use for the output file */
OUTPUT_FORMAT(elf32-i386)
/* specify a paricular output machine architecture */
OUTPUT_ARCH(i386:i386)

/* the SECTIONS command tells the linker how to map input sections into output sections, and how to place the output sections in memory */
SECTIONS
{
    . = 0x0100000;
    .text :
    {
        *(.multiboot)
        *(.text)
        /* read only data */
        *(.rodata)
    }

    .data :
    {
        _start_ctors = .;
        /* .init_array
         * global object generated by constructor in .init_array
         * use KEEP to forbid releasing the resources
         */
        KEEP(*(.init_array));
        /* SORT_BY_INIT_PRIORITY will sort sections into ascending order by 
         * numerical value of the GCC init_priority attribute encoded in the 
         * section name before placing them in the output file 
         */
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array)));
        _end_ctors = .;
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.fini_array)
        *(.comment)
    }
}
```

## 1.5 操作系统的引导过程

有了linker.ld脚本文件，就可以执行链接的过程。在Makefile中加入链接过程。
``` makefile
# /os/Makefile

# ...

%.o: %.s
	as ${ASPARAMS} -o $@ $<

mykernel.bin: linker.ld ${objects}
	ld ${LDPARAMS} -T $< -o $@ ${objects}
```

我们使用VMware Workstation启动操作系统，VMware Workstation的测试版本为16.0.0，它使用操作系统镜像启动一个操作系统。Linux有制作镜像的工具，在命令行中使用下面命令安装：
``` bash
sudo apt install xorriso grub-efi-amd64 grub-pc
```
安装完毕以后可以使用指令`grub-mkrescue`创建iso镜像。

Multiboot规范规定GRUB根据/boot/grub/grub.conf文件查找Kernel信息并加载Kernel程序，grub.conf的信息如下：
```
set timeout=0
set default=0
menuentry "my os" {
    multiboot /boot/mykernel.bin
    boot
}
```
我们在Makefile文件中创建这个文件，添加代码如下：
``` makefile
# os/Makefile
# ...
mykernel.bin: linker.ld ${objects}
	ld ${LDPARAMS} -T $< -o $@ ${objects}

mykernel.iso: mykernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "my os" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/mykernel.bin' >> iso/boot/grub/grub.cfg
	echo '    boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	rm -rf iso

clean:
	rm kernel.o loader.o mykernel.bin mykernel.iso
```
在命令行中执行
```
make mykernel.o
```
可以看到在os目录下生成了mykernel.iso文件。

打开VMware Workstation并选择“文件 - 新建虚拟机 - 典型 - 安装程序光盘映像文件 - 选择镜像文件所在的路径”。操作系统类型更改为“其他”，内存分配64M，不需要创建磁盘。创建完毕后开启虚拟机可以看到屏幕上的`hello world`。

## 1.6 关于printf的问题

之前`printf`的一个问题是每次打印字符都从固定的位置`0xb8000`开始，下一次打印将覆盖上一次的内容。
```
unsigned short *VideoMemory = (unsigned short*)0xb8000;
```
为了解决问题，需要对整块屏幕进行字符位置的控制。屏幕的大小是`80 x 25`，每输出80个字节需要进行换行，每满25行需要清屏，打印位置重置为第一行行首。然而屏幕在内存中的地址是连续的，因此可以用`VideoMemory[80 * y + x]`来访问第`y`行第`x`位。如果遇到换行符，则直接进行换行。

``` cpp
// os/kernel.cpp
void printf(const char *str)
{
    // screen address
    static uint16_t *VideoMemory = (uint16_t*)0xb8000;
    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i]; i++)
    {
        switch (str[i])
        {
            case '\n':
                y++;
                x = 0;
                break;
            default:
                VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
                x++;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }
        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
            {
                for (x = 0; x < 80; x++)
                    VideoMemory[i] = (VideoMemory[i] & 0xFF00) | ' ';
            }
            x = 0;
            y = 0;
        }
    }
}
```
