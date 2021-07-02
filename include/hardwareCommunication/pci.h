#ifndef __PCI_H__
#define __PCI_H__

#include "common/types.h"
#include "port.h"
#include "interrupts.h"
#include "drivers/driver.h"

namespace zoeos
{

namespace hardwareCommunication
{
    class PciController;

    class BaseAddressRegister
    {
        friend class PciController;
    public:
        enum Type { MMIO = 0, IO = 1 };

    private:
        Type type;
        bool prefetchable;
        uint8_t *address;
        uint32_t size;
    };

    class PciConfigSpace
    {
        friend class PciController;
    public:
        PciConfigSpace();
        PciConfigSpace(uint8_t bus_, uint8_t device_,
                uint8_t function_, uint16_t vendorID_,
                uint16_t deviceID_, uint8_t classCode_,
                uint8_t subclass_, uint8_t progIF_, 
                uint8_t revision_, uint32_t interrupt_, uint32_t portBase_ = 0);
        ~PciConfigSpace();

    private:
        uint32_t portBase;
        uint32_t interrupt;

        uint8_t bus;
        uint8_t device;
        uint8_t function;

        uint16_t deviceID;
        uint16_t vendorID;
        uint8_t classCode;
        uint8_t subclass;
        uint8_t progIF;
        uint8_t revision;
    };

    class PciController
    {
    public:
        PciController();
        ~PciController();

        uint32_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

        void pciConfigWriteWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);

        bool isMultiFuncDevice(uint8_t bus, uint8_t device);

        void checkBuses(drivers::DriverManager *driverManager, InterruptManager *interrupts);
        PciConfigSpace getConfigSpace(uint8_t bus, uint8_t device, uint8_t function);

        drivers::Driver *getDriver(PciConfigSpace device, InterruptManager *interrupts);

        BaseAddressRegister getBaseAddressRegister(uint8_t bus, uint8_t device, uint8_t function, uint8_t num);

    private:
        Port32Bit dataPort;
        Port32Bit addressPort;
    };
}

}

#endif