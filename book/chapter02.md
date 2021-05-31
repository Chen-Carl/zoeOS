# 使用C/C++实现一个操作系统

# chapter 02 内存管理

## 2.1 准备工作

为了方便定义整型类型，在头文件types.h中引入如下定义：
``` cpp
// os/types.h
#ifndef __TYPES_H__
#define __TYPES_H__

typedef char int8_t;
typedef unsigned char uint8_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;

#endif
```

并修改之前的kernel.cpp中整型声明的内容为
``` cpp
// os/kernel.cpp
#include "types.h"
// ...

void printf(char *str)
{
    // ...
    static uint16_t *VideoMemory = (uint16_t*)0xb8000;
    // ...
}

void kernelMain(void *multiboot_structure, uint32_t magicnumber)
// ...
```

## 2.2 实现GDT类

GDT中的表项是段描述符，因此首先在GDT类中定义段描述符类。x86的段描述符格式如下所示。

|基址24~31|G|D|0||Limit 16~19|P|DPL|S|Type|基址16~23|
|-|-|-|-|-|-|-|-|-|-|-|
|-|0:Limit单位为字节<br>1:Limit单位为页面|1:16位段<br>0:32位段|-|-|-|0:段不在内存中<br>1:段在内存中|特权级0~3|0:系统程序<br>1:应用程序|段类型及保护|

|基址0~15|Limit 0~15|
|-|-|

Limit, Base, type都分拆为多个部分存储，其中16~19位的Limit字段所在的字节还包括了其它的一些标志，type与此类似。为了方便操作，在类中实现时，都将它们定义为`uint8_t`类型。

### SegmentDescriptor类

构造函数中需要传入基址`base`，段最大长度`limit`和段类型`type`。两个方法`Base()`和`Limit()`返回基址和段长度。私有成员变量如注释所示。

在`SegmentDescriptor`类定义的最后需要告诉编译器取消内存对齐的优化来保证描述符满足x86格式，这是GCC特有的语法。由于需要手动内存访问，因此每个变量定义的顺序就变得十分重要，这里按照x86段描述符从低位到高位的顺序定义每个字段。

这样一来，每个段描述符的大小都是8字节。

### GlobalDescriptorTable类

GDT中包含了4个基本的段，分别是空段、未使用段、代码段和数据段。同时定义了两个函数`getCodeSegmentSelector`和`getDataSegmentSelector`返回代码段和数据段相对于GDT表地址的偏移量。由于描述符都是8字节的，因此这个偏移量的后3位恒为0。实际上，这个偏移量的高11位是段选择子的高11位，我们这里返回段选择子的高11位的内容。

``` cpp
// os/gdt.h
#ifndef __GDT_H__
#define __GDT_H__

#include "types.h"

class GlobalDescriptorTable
{
public:
    class SegmentDescriptor
    {
    public:
        // base: base address
        // limit: max segment length
        // type: segment type, eg. data segment, code segment etc.
        SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
        uint32_t Base();
        uint32_t Limit();
    private:
        uint16_t limit_lo;          // limit字段低位
        uint16_t base_lo;           // base字段低位
        uint8_t base_hi;            // base字段次高位
        uint8_t type;               // type字段和其他标志
        uint8_t flags_limit_hi;     // limit字段高位和其他标志
        uint8_t base_vhi;           // base字段最高位
    } __attribute__((packed));

    SegmentDescriptor nullSegmentDescriptor;
    SegmentDescriptor unusedSegmentDescriptor;
    SegmentDescriptor codeSegmentDescriptor;
    SegmentDescriptor dataSegmentDescriptor;

    GlobalDescriptorTable();
    ~GlobalDescriptorTable();

    uint16_t getCodeSegmentSelector();
    uint16_t getDataSegmentSelector();
}

#endif
```

x86描述符使用20位能够表示32位的地址空间。如果G位域是0，则是精确到字节的段长度，最大1MB；如果是1，段长度域以页面替代字节作为单元给出段的大小。对于4KB页面段大小，20位足够最大32位的段使用。由于在定义类时没有要求内存对齐，因此可以使用类的起始地址`target`方便地访问类中的任何数据。将`target`定义为`uint8_t`，则它的跨度就是1字节。

``` cpp
GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type)
{
    // 获取描述符首地址
    uint8_t *target = (uint8_t *)this;
    // 判断是否需要使用页作为最小单位
    // 如果段的字节数不足1048576，则最小单位是字节，否则为页
    if (limit < 1048576)
    {
        // 寻找到flags_limit_hi字段
        // 将最小单位设置为字节
        // 使用32位段
        target[6] = 0x40;
    }
    else
    {
        // 将最小单位设置为页
        target[6] = 0xc0;
        // 如果最后4KB不足则需要缺失一页
        // 将limit转化为4KB的页面数
        if ((limit & 0xfff) != 0xfff)
            limit = (limit >> 12) - 1;
        else
            limit = limit >> 12;
    }
    // 此时limit是20位的
    // limit字段低8位取limit的后8位
    target[0] = limit & 0xff;
    // limit字段低16位的后8位
    target[1] = (limit >> 8) & 0xff;
    // limit字段高位
    target[6] = (limit >> 16) & 0xf | 0xc0;

    // base字段低16位的前8位
    target[2] = base & 0xff;
    // base字段低16位的后8位
    target[3] = (base >> 8) & 0xff;
    // base字段的次高位
    target[4] = (base >> 16) & 0xff; 
    // base字段的最高位
    target[7] = (base >> 24) & 0xff; 

    // type字段
    target[5] = type;
}
```

`Base()`和`Limit()`也使用这种技巧。

``` cpp
uint32_t GlobalDescriptorTable::SegmentDescriptor::Base()
{
    // 获取当前对象的首地址
    uint8_t *target = (uint8_t *)this;
    // 从最高位开始获取，并进行左移
    uint32_t result = target[7];
    result = (result << 8) + target[4];
    result = (result << 8) + target[3];
    result = (result << 8) + target[2];
    return result;
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit()
{
    uint8_t *target = (uint8_t *)this;
    uint32_t result = target[6] & 0xf;
    result = (result << 8) + target[1];
    result = (result << 8) + target[0];
    if ((target[6] & 0xc0) != 0xc0)
        result = (result << 12) | 0xfff;
    return result;
}
```

建立gdt.cpp文件，并引入上面的函数。
``` cpp
// os/gdt.cpp
#include "gdt.h"

GlobalDescriptorTable::SegmentDescriptor::SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type)
{
    // ...
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Base()
{
    // ...
}

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit()
{
    // ...
}
```

接下来是`GlobalDescriptorTable`的构造函数和析构函数。有关gcc内联汇编的知识请见[gcc内联汇编](http://csapp.cs.cmu.edu/3e/waside/waside-embedded-asm.pdf)。内联汇编的扩展格式为
```c
asm(assembler template
    : output operands                   /* optional */
    : input operands                    /* optional */
    : list of clobbered registers       /* optional */
    );
```

构造函数中首先构造4个段描述符对象。构造`nullSegmentDescriptor`和`unusedSegmentDescriptor`都传入空值。代码段和数据段的段大小都是64M。最后一个`type`参数分别意味着
```
codeSegmentDescriptor: 0x9a = 1001 1010
dataSegmentDescriptor: 0x92 = 1001 0010
```
前4位分别表示两个段都存储在内存中，特权级最高，是系统程序。后4位每一位都有对应的含义，如下表所示：

|是否是代码段|用于内存保护|是否可读|是否开启调试|
|-|-|-|-|
|1：代码段|置0|1：可读|置0，由CPU自动设置|

CPU必须知道GDT的入口，也就是基地址放在哪里，Intel的设计者提供了一个寄存器`GDTR`用来存放GDT的入口地址，程序员将GDT设定在内存中某个位置之后，可以通过`lgdt`指令将GDT的入口地址装入此寄存器，从此以后，CPU就根据此寄存器中的内容作为GDT的入口来访问GDT了。GDTR中存放的是GDT在内存中的基地址和其表长界限，分别是32位和16位。

|Base(16~47)|Size(0~15)|
|-|-|
|32|16|

在构造函数中，`i[0]`是一个32位的变量，将它的高16位和`i[1]`组合在一起作为GDTR，因此`i[1]`存放GDT的首地址，`i[0]`的高16位存放GDT大小。使用`lgdt`指令访问时，访问的是`i[0]`高16位的起始地址，使用强制类型转换更改指针步长以满足这个要求。

``` cpp
GlobalDescriptorTable::GlobalDescriptorTable()
    : nullSegmentDescriptor(0, 0, 0),
      unusedSegmentDescriptor(0, 0, 0),
      codeSegmentDescriptor(0, 64 * 1024 * 1024, 0x9a),
      dataSegmentDescriptor(0, 64 * 1024 * 1024, 0x92)
{
    uint32_t i[2];
    i[0] = sizeof(GlobalDescriptorTable) << 16;
    i[1] = (uint32_t)this;
    asm volatile("lgdt %[p]"
            :                               /* outputs */
            : [p] "r" (((uint8_t *)i) + 2)  /* inputs */
            );
}

GlobalDescriptorTable::~GlobalDescriptorTable() { }
```

两个获取相对GDT表地址偏移量的方法`DataSegmentDescriptor()`和`CodeSegmentDescriptor()`十分简单，由于要获取字节为单位的偏移量，需要将指针的步长转化成`uint8_t`单位。
``` cpp
// os/gdt.cpp
#include "gdt.h"

// ...

uint32_t GlobalDescriptorTable::SegmentDescriptor::Limit()
{
    // ...
}

GlobalDescriptorTable::GlobalDescriptorTable()
// ...

GlobalDescriptorTable::~GlobalDescriptorTable() { }

uint16_t GlobalDescriptorTable::getDataSegmentSelector()
{
    return ((uint8_t *)&dataSegmentDescriptor - (uint8_t *)this) >> 3;
}

uint16_t GlobalDescriptorTable::getCodeSegmentSelector()
{
    return ((uint8_t *)&codeSegmentDescriptor - (uint8_t *)this) >> 3;
}
```

到这里为止，GDT类已经完成。