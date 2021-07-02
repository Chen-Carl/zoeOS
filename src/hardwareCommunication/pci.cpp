#include "hardwareCommunication/pci.h"

using namespace zoeos::common;
using namespace zoeos::hardwareCommunication;
using namespace zoeos::drivers;

void printf(const char *);
void printHex(uint8_t );

PciConfigSpace::PciConfigSpace() { }

PciConfigSpace::PciConfigSpace(uint8_t bus_, uint8_t device_,
                uint8_t function_, uint16_t vendorID_,
                uint16_t deviceID_, uint8_t classCode_,
                uint8_t subclass_, uint8_t progIF_, 
                uint8_t revision_, uint32_t interrupt_,
                uint32_t portBase_)
{
    bus = bus_;
    device = device_;
    function = function_;

    vendorID = vendorID_;
    deviceID = deviceID_;
    classCode = classCode_;
    subclass = subclass_;
    progIF = progIF_;
    revision = revision_;
    interrupt = interrupt_;
    portBase = portBase_;
}

PciConfigSpace::~PciConfigSpace() { }

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

void PciController::pciConfigWriteWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t CONFIG_ADDRESS = 1 << 31 | ((bus & 0xff) << 16) |
            ((slot & 0x1f) << 11) | 
            ((func & 0x07) << 8) |
            (offset & 0xfc);
    addressPort.write(CONFIG_ADDRESS);
    dataPort.write(value);
}

bool PciController::isMultiFuncDevice(uint8_t bus, uint8_t device)
{
    return pciConfigReadWord(bus, device, 0, 0x0e) & (1 << 7);
}

PciConfigSpace PciController::getConfigSpace(uint8_t bus, uint8_t device, uint8_t function)
{
    uint16_t vendorID = pciConfigReadWord(bus, device, function, 0);
    uint16_t deviceID = pciConfigReadWord(bus, device, function, 0x02);

    uint8_t classCode = pciConfigReadWord(bus, device, function, 0x0b);
    uint8_t subclass = pciConfigReadWord(bus, device, function, 0x0a);
    uint8_t progIF = pciConfigReadWord(bus, device, function, 0x09);
    uint8_t revision = pciConfigReadWord(bus, device, function, 0x08);

    uint32_t interrupt = pciConfigReadWord(bus, device, function, 0x3c);

    return PciConfigSpace(bus, device, function, vendorID, deviceID, classCode, subclass, progIF, revision, interrupt);
}

void PciController::checkBuses(drivers::DriverManager *driverManager, InterruptManager *interrupts)
{
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            int functionNumber = isMultiFuncDevice(bus, device) ? 8 : 1;
            for (uint8_t function = 0; function < functionNumber; function++)
            {
                PciConfigSpace deviceConfigSpace = getConfigSpace(bus, device, function);
                if (deviceConfigSpace.vendorID == 0 || deviceConfigSpace.vendorID == 0xffff)
                    continue;
                
                printf("PCI BUS ");
                printHex(bus & 0xff);

                printf(", DEVICE ");
                printHex(device);

                printf(", FUNCTION ");
                printHex(function);

                printf(" = VENDOR ");
                printHex((deviceConfigSpace.vendorID & 0xff00) >> 8);
                printHex(deviceConfigSpace.vendorID & 0xff);

                printf(", DEVICE ");
                printHex((deviceConfigSpace.deviceID & 0xff00) >> 8);
                printHex(deviceConfigSpace.deviceID & 0xff);
                printf("\n");

                for (uint8_t num = 0; num < 6; num++)
                {
                    BaseAddressRegister BAR = getBaseAddressRegister(bus, device, function, num);
                    if (BAR.address && (BAR.type == BaseAddressRegister::Type::IO))
                    {
                        deviceConfigSpace.portBase = (uint32_t)BAR.address;
                    }
                    Driver *driver = getDriver(deviceConfigSpace, interrupts);
                    if (driver)
                    {
                        driverManager->addDriver(driver);
                    }
                }
            }
        }
    }
}

Driver *PciController::getDriver(PciConfigSpace device, InterruptManager *interrupts)
{
    switch (device.vendorID)
    {
        case 0x1022:
        {
            if (device.deviceID == 0x2000)
            {
                printf("AMD");
                break;
            }
        }
        case 0x8086:
            break;
    }

    switch (device.classCode)
    {
        case 0x03:
        {
            if (device.subclass == 0x00)
            {
                printf("VGA");
                break;
            }
            break;
        }
    }
    return 0;
}

BaseAddressRegister PciController::getBaseAddressRegister(uint8_t bus, uint8_t device, uint8_t function, uint8_t num)
{
    BaseAddressRegister res;
    uint32_t headerType = pciConfigReadWord(bus, device, function, 0x0e) & 0x7e;
    int numOfBAR = 6 - 4 * headerType;
    // 返回空对象
    if (num >= numOfBAR)
        return res;

    uint32_t attribute = pciConfigReadWord(bus, device, function, 0x10 + 4 * num);
    res.type = (attribute & 1) ? BaseAddressRegister::Type::IO : BaseAddressRegister::Type::MMIO;

    if (res.type == BaseAddressRegister::Type::MMIO)
    {
        switch ((attribute >> 1) & 0x3)
        {
            case 0: 
            case 1: 
            case 2:
                break;
        }
    }
    else
    {
        res.address = (uint8_t*)(attribute & ~0x3);
        res.prefetchable = false;
    }
    return res;
}

