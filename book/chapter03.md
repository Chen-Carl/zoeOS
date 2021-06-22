# 使用C/C++实现一个操作系统

# chapter 03 I/O端口

操作系统需要实现鼠标、键盘等硬件设备的相关操作，涉及到硬件编程，控制硬件是通过操作硬件芯片端口实现的。

我们将编写一个I/O端口类来实现端口管理，包括对8bit，16bit和32bit的端口。

``` cpp
// os/port.h
#ifndef __PORT_H__
#define __PORT_H__

#include "types.h"

class Port 
{
protected:
    uint16_t portnumber;
    Port(uint16_t portnumber);
    ~Port();
};

class Port8Bit : public Port 
{
public:
    Port8Bit(uint16_t portnumber);
    ~Port8Bit();
    virtual void write(uint8_t data);
    virtual uint8_t read();
};

class Port16Bit : public Port
{
public:
    Port16Bit(uint16_t portnumber);
    ~Port16Bit();
    virtual void write(uint16_t data);
    virtual uint16_t read();
};

class Port32Bit : public Port
{
public:
    Port32Bit(uint16_t portnumber);
    ~Port32Bit();
    virtual void write(uint32_t data);
    virtual uint32_t read();
};

#endif
```

> 正常情况下，不同数据类型的`Port`类应当使用类模板实现，但是问题在于编写端口操作的实现需要使用汇编代码，而不同类型的端口操作中，汇编的指令是不同的，这不能使用模板参数给定。这种关系更加类似于`is-a`关系，故采用公有继承。

对端口的读写操作的汇编指令分别为`in`和`out`，后缀`bwl`表示端口的位数。这里同样使用了内联汇编的写法。

``` cpp
// os/port.cpp
#include "port.h"

Port::Port(uint16_t portnumber)
    : portnumber(portnumber) {}

Port::~Port() {}

Port8Bit::Port8Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port8Bit::~Port8Bit() {}

void Port8Bit::write(uint8_t data)
{
    __asm__ volatile("outb %0, %1"
                     :
                     : "a"(data), "Nd"(portnumber));
}

uint8_t Port8Bit::read()
{
    uint8_t result;
    __asm__ volatile("inb %1, %0"
                     : "=a"(result)
                     : "Nd"(portnumber)
    );
    return result;
}

void Port8BitSlow::write(uint8_t data)
{
    __asm__ volatile("outb %0, %1\njmp 1f\n1: jmp 1f\n1:"
                     : 
                     : "a"(data), "Nd"(portnumber)
    );
}

Port16Bit::Port16Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port16Bit::~Port16Bit() {}

void Port16Bit::write(uint16_t data)
{
    __asm__ volatile("outw %0, %1"
                     :
                     : "a"(data), "Nd"(portnumber)
    );
}

uint16_t Port16Bit::read()
{
    uint16_t result;
    __asm__ volatile("inw %1, %0"
                     : "=a"(result)
                     : "Nd"(portnumber));
    return result;
}

Port32Bit::Port32Bit(uint16_t portnumber)
    : Port(portnumber) {}

Port32Bit::~Port32Bit() {}

void Port32Bit::write(uint32_t data)
{
    __asm__ volatile("outl %0, %1"
                     :
                     : "a"(data), "Nd"(portnumber)
    );
}

uint32_t Port32Bit::read()
{
    uint32_t result;
    __asm__ volatile("inl %1, %0"
                     : "=a"(result)
                     : "Nd"(portnumber));
    return result;
}

```

最后需要添加一个“慢写”的操作。

``` cpp
// os/port.h
class Port8BitSlow : public Port8Bit
{
public:
    Port8BitSlow(uint16_t portnumber);
    ~Port8BitSlow();
    virtual void write(uint8_t data);
};
```

``` cpp
// os/port.cpp
Port8BitSlow::Port8BitSlow(uint16_t portnumber)
    : Port8Bit(portnumber) {}

Port8BitSlow::~Port8BitSlow() {}

void Port8BitSlow::write(uint8_t data)
{
    __asm__ volatile("outb %0, %1\njmp 1f\n1: jmp 1f\n1:"
                     : 
                     : "a"(data), "Nd"(portnumber)
    );
}
```

至此为止，我们可以用`Port`类指定一个端口并通过该端口读写数据。