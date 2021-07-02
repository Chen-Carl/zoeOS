# 使用C/C++实现一个操作系统

# chapter 06 PCI总线

## 6.1 PCI总线介绍

### 一、PCI总线结构

总线就是一条共享的通信链路，它用一套线路来连接多个子系统。PCI总线独立于CPU总线，可以和CPU总线并行操作。

PCI总线上可以挂接PCI设备和PCI桥片，PCI总线上只允许有一个PCI主设备，其他的均为PCI从设备，而且读写操作只能在主从设备之间进行，从设备之间的数据交换需要通过主设备中转。

总线结构的两个主要优点是功能多和成本低。通过定义一种连接方案，就能够方便的添加新设 备，比如我们可以轻松的为自己的笔记本扩展内存条，或者外扩一块硬盘。外围设备也可以在使用同类总线的计算机系统之间移动。

总线的主要缺点在于它会产生通信瓶颈，这可能会限制I/O的最大吞吐量。当I/O数据传输必须通过单个总线的时候，这条总线的带宽就会限制I/O的最大吞吐量。

### 二、控制线路和数据线路

总线通常包括一套控制线路和一套数据线路。控制线路用来传输请求和确认信号，并指出数据线上的信息类型。总线的数据线在源和目的之间传递信息。这种信息可能包括数据、复杂指令或者地址。

制总线都要遵循总线协议，并指出总线传输的内容。由于总线的共享的，所以我们还需要一个协议来决定谁下一个使用。

## 6.2 PCI配置空间

以下内容参考自[https://wiki.osdev.org/Pci](https://wiki.osdev.org/Pci)。

### 一、配置空间结构

PCI有三个相互独立的物理地址空间：设备存储器地址空间、I/O地址空间和配置空间。配置空间是PCI所特有的一个物理空间。由于PCI支持设备即插即用，所以PCI设备不占用固定的内存地址空间或I/O地址空间，而是由操作系统决定其映射的基址。

系统加电时，BIOS检测PCI总线，确定所有连接在PCI总线上的设备以及它们的配置要求，并进行系统配置。所以，所有的PCI设备必须实现配置空间，从而能够实现参数的自动配置，实现真正的即插即用。

PCI总线规范定义的配置空间总长度为256个字节，配置信息按一定的顺序和大小依次存放。前64个字节的配置空间称为配置头，对于所有的设备都一样，配置头的主要功能是用来识别设备、定义主机访问PCI卡的方式（I/O访问或者存储器访问，还有中断信息）。其余的192个字节称为本地配置空间（设备有关区），主要定义卡上局部总线的特性、本地空间基地址及范围等。

配置空间中最重要的有：

- Vendor ID：厂商ID。知名的设备厂商的ID。FFFFh是一个非法厂商ID，可它来判断PCI设备是否存在。

- Device ID：设备ID。某厂商生产的设备的ID。操作系统就是凭着 Vendor ID和Device ID 找到对应驱动程序的。

- Class Code：类代码。共三字节，分别是 类代码、子类代码、编程接口。类代码不仅用于区分设备类型，还是编程接口的规范，这就是为什么会有通用驱动程序。

- Interrupt Line：IRQ编号。PC机以前是靠两片8259芯片来管理16个硬件中断。现在为了支持对称多处理器，有了APIC（高级可编程中断控制器），它支持管理24个中断。

- Interrupt Pin：中断引脚。PCI有4个中断引脚，该寄存器表明该设备连接的是哪个引脚。

### 二、访问配置空间

可通过访问`0xCF8h`、`0xCFCh`访问配置空间。
```
0xCF8h: CONFIG_ADDRESS  PCI配置空间地址端口
0xCFCh: CONFIG_DATA     PCI配置空间数据端口
```

CONFIG_ADDRESS寄存器格式为

|31|30 - 24|23 - 16|15 - 11|10 - 8|7 - 0|
|-|-|-|-|-|-|
|Enable Bit|Reserved|Bus Number|Device Number|Function Number|Register Offset|

之前我们实现了`Port`类，可以方便地访问端口。我们定义函数`pciConfigReadWord()`用于访问配置空间，它读取配置空间中的一个32位字段，即一个寄存器的值。为了对PCI总线进行管理，将对PCI总线的控制统一封装在类`PciController`中。

``` cpp
// os/include/hardwareCommunication/pci.h
#ifndef __PCI_H__
#define __PCI_H__

#include "common/types.h"
#include "port.h"
#include "interrupts.h"

namespace zoeos
{

namespace hardwareCommunication
{
    class PciController
    {
    public:
        PciController();
        ~PciController();

        uint32_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

    private:
        Port32Bit dataPort;
        Port32Bit addressPort;
    };
}

}

#endif
```

我们在构造函数中传递配置空间的地址端口和数据端口以便访问配置空间。我们在地址端口写入`CONFIG_ADDRESS`来告诉数据端口我们需要的是哪条PCI总线、哪个设备、哪个功能的配置空间，并通过`offset`指定读取的字段。这里的`offset`实际上是`Register Offset`，它的含义是所要读取的字段在配置空间中的偏移量。而配置空间中每32位字段都存放在一个寄存器中，我们读取到的字段在寄存器中的偏移实际上是`offset % 4`，为了获取32位的字段值中的8位，需要对这个寄存器中的值进行右移操作。
``` cpp
return result >> (8 * (offset % 4));
```

完整代码如下。
``` cpp
// os/src/hardwareCommunication/pci.cpp
#include "hardwareCommunication/pci.h"

using namespace zoeos::common;
using namespace zoeos::hardwareCommunication;

PciController::PciController() : dataPort(0xcfc), addressPort(0xcf8) { }

PciController::~PciController() { }

uint32_t PciController::pciConfigReadWord (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t CONFIG_ADDRESS = 1 << 31 | ((bus & 0xff) << 16) |
                  ((slot & 0x1f) << 11) | 
                  ((func & 0x07) << 8) |
                  (offset & 0xfc);
    addressPort.write(CONFIG_ADDRESS);
    uint32_t result = dataPort.read();
    return result >> (8 * (offset % 4));
}
```

写操作和读操作相似。
``` cpp 
// os/src/hardwareCommunication/pci.cpp
// ...
void PciController::pciConfigWriteWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t CONFIG_ADDRESS = 1 << 31 | ((bus & 0xff) << 16) |
            ((slot & 0x1f) << 11) | 
            ((func & 0x07) << 8) |
            (offset & 0xfc);
    addressPort.write(CONFIG_ADDRESS);
    dataPort.write(value);
}
```

为了判断一个设备是否是多功能设备，我们关注`Header Type`字段。PCI规范规定，如果第7为为1，那么该设备存在8个功能，否则它是一个单功能设备。函数`isMultiFuncDevice()`需要指定总线和设备号作为参数。
``` cpp
// os/src/hardwareCommunication/pci.cpp
// ...
bool PciController::isMultiFuncDevice(uint8_t bus, uint8_t device)
{
    return pciConfigReadWord(bus, device, 0, 0x0e) & (1 << 7);
}
```

把上面的函数都加入到头文件中，头文件如下。
``` cpp
// os/include/hardwareCommunication/pci.h
// ...
namespace hardwareCommunication
{
    class PciController
    {
    public:
        // ...
        void PciController::pciConfigReadWrite(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
        bool isMultiFuncDevice(uint8_t bus, uint8_t device);
    // ...
    };
}
// ...
```

既然我们可以访问设备的配置空间了，我们就能够获取一个设备的配置空间对象。定义配置空间类，将有关信息都存储在这个类中。
``` cpp 
// os/include/hardwareCommunication/pci.h
class PciConfigSpace
{
public:
    PciConfigSpace();
    PciConfigSpace(uint32_t interrupt_, uint8_t bus_, 
                    uint8_t device_, uint8_t function_,
                    uint32_t deviceID_, uint32_t vendorID_,
                    uint8_t classCode_, uint8_t subclass_,
                    uint8_t progIF_, uint8_t revision_,
                    uint32_t portBase_ = 0);
    ~PciConfigSpace();

private:
    uint32_t portBase;
    uint32_t interrupt;

    uint8_t bus;
    uint8_t device;
    uint8_t function;

    uint32_t deviceID;
    uint32_t vendorID;
    uint8_t classCode;
    uint8_t subclass;
    uint8_t progIF;
    uint8_t revision;
};

class PciController
{
    // ...
}
```

为它提供两个重载的构造函数和一个析构函数。
``` cpp 
// os/src/hardwareCommunication/pci.cpp
PciConfigSpace::PciConfigSpace() { }
PciConfigSpace::PciConfigSpace(uint32_t interrupt_, uint8_t bus_, 
                uint8_t device_, uint8_t function_,
                uint32_t deviceID_, uint32_t vendorID_,
                uint8_t classCode_, uint8_t subclass_,
                uint8_t progIF_, uint8_t revision_,
                uint32_t portBase_ = 0)
{
    interrupt = interrupt_;
    bus = bus_;
    device = device_;
    function = function_;
    deviceID = deviceID_;
    vendorID = vendorID_;
    classCode = classCode_;
    subclass = subclass_;
    progIF = progIF_;
    revision = revision_;
    portBase = portBase_;
}

PciConfigSpace::~PciConfigSpace() { }
```

在PCI控制器中，实现函数`getConfigSpace()`用于获取特定设备特定功能的配置空间。
``` cpp
// os/include/hardwareCommunication/pci.h
class PciController
{
public:
    PciConfigSpace getConfigSpace(uint8_t bus, uint8_t device, uint8_t function);
    // ...
}
```
既然我们已经能从配置空间中读取想要的内容了，我们只需要把相应的内容赋值给配置空间对象中的特定成员即可。
``` cpp 
// os/src/hardwareCommunication/pci.cpp
PciConfigSpace PciController::getConfigSpace(uint8_t bus, uint8_t device, uint8_t function)
{
    uint8_t vendorID = pciConfigReadWord(bus, device, function, 0);
    uint8_t deviceID = pciConfigReadWord(bus, device, function, 0x02);
    uint8_t classCode = pciConfigReadWord(bus, device, function, 0x0b);
    uint8_t subclass = pciConfigReadWord(bus, device, function, 0x0a);
    uint8_t progIF = pciConfigReadWord(bus, device, function, 0x09);
    uint8_t revision = pciConfigReadWord(bus, device, function, 0x08);
    uint8_t interrupt = pciConfigReadWord(bus, device, function, 0x3c);
    return PciConfigSpace(interrupt, bus, device, function, deviceID, vendorID, classCode, subclass, progIF, revision);
}
```

## 6.3 扫描PCI设备

PCI有256条总线，每条总线可以连接32个设备，每个设备最多有8个功能。`checkBuses()`函数通过遍历扫描PCI的所有设备，并将设备的设备名、厂商名打印出来。

注意，每个设备驱动是和中断相联系的，因此需要同时传入参数`driverManager`和`interrupts`。

``` cpp
// os/src/hardwareCommunication/pci.cpp
void PciController::checkBuses(drivers::DriverManager *driverManager, InterruptManager *interrupts)
{
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            int functionNumber = isMultiFuncDevice(bus, device) ? 8 : 1;
            for (uint8_t function = 0; function < functionNumber; function++)
            {
                PciConfigSpace deviceDescriptor = getConfigSpace(bus, device, function);
                if (deviceDescriptor.vendorID == 0 || deviceDescriptor.vendorID == 0xffff)
                    continue;
                
                printf("PCI BUS ");
                printHex(bus & 0xff);

                printf(", DEVICE ");
                printHex(device);

                printf(", FUNCTION ");
                printHex(function);

                printf(" = VENDOR ");
                printHex((deviceDescriptor.vendorID & 0xff00) >> 8);
                printHex(deviceDescriptor.vendorID & 0xff);

                printf(", DEVICE ");
                printHex((deviceDescriptor.deviceID & 0xff00) >> 8);
                printHex(deviceDescriptor.deviceID & 0xff);
                printf("\n");
            }
        }
    }
}
```

## 6.4 基址寄存器

配置空间中存在6个基址寄存器。每个 PCI 设备内部都会有一部分资源需要提供给系统软件访问，不同的 PCI 设备可供系统软件访问的资源大小、资源类型也不一样。基址寄存器就是 PCI 协议提供的、用于向系统软件展示 PCI 设备内部资源大小、资源类型和资源属性的机制。

### 一、基址寄存器的结构

基址寄存器的第 0 位用于表示 PCI 设备内部资源的类型。PCI 设备制造商在生产该设备时会根据实际情况配置该值。对于系统软件来说，该位是只读的。

- bit 0 = 1 :  系统软件需要在I/O空间中为该资源分配地址
- bit 0 = 0 :  系统软件需要在内存映射I/O空间中为该资源分配地址

两者分别简记为I/O类型和MMIO类型。

如果资源是MMIO类型，则第1-3位有效。前两位为：
- 00表示系统软件需要为该资源分配一个32位的地址
- 10表示系统软件需要为该资源分配一个64为的地址

如果需要分配64位的地址，相邻两个基址寄存器会合并成1个使用，后者保存64位地址的高32位。

第3位为：
- 1 表示该资源是可预取的
- 0 表示该资源是不可预取的

可预取MMIO的特点是读没有副作用，且多笔写事务可以合并为一笔。预取允许提前将数据取到缓存中，但是类似于状态寄存器资源是不允许预取的。

如果资源是I/O类型的，则这些位无效。

### 二、资源大小

bit 7 开始的位用于指示资源大小。bit 4 ~ 6 必须设置为0，且不可更改。

对于64为地址的寄存器，高32位全部用来指示资源大小。

对于 PCIe 设备来说，并不是说有的基址寄存器都是有效的。

例如，某个 PCI Endpoint 设备内部只有 2 个资源可以供系统软件访问，那么该设备制造商只要使用 BAR0 和 BAR1 两个基址寄存器就可以了。其他的 BAR2 ~ BAR5 都可以全部设置为0，并且不允许系统软件修改。

### 三、基址寄存器类

根据上面的描述可以定义基址寄存器类`BaseAddressRegister`。`address`表示配置空间的起始地址，再根据`offset`的大小获得某个基址寄存器。

``` cpp
// os/src/hardwareCommunication/pci.cpp
class BaseAddressRegister
{
public:
    enum Type { MMIO = 0, IO = 1 };

private:
    Type type;
    bool prefetchable;
    uint8_t *address;
    uint32_t offset;
};
```

最终的目的是获取特定设备的驱动，即为函数`Driver *PciController::getDriver(PciConfigSpace device, InterruptManager *interrupts)`，并且将驱动和中断相联系。

从基址寄存器中可以获取资源相关的信息，获取基址寄存器的操作为`BaseAddressRegister PciController::getBaseAddressRegister(uint8_t bus, uint8_t device, uint8_t function, uint8_t num)`。这两个函数可根据需要实现。