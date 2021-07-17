#ifndef __AMD_AM79C973_H__
#define __AMD_AM79C973_H__

#include "common/types.h"
#include "drivers/driver.h"
#include "hardwareCommunication/pci.h"
#include "hardwareCommunication/interrupts.h"
#include "hardwareCommunication/port.h"

namespace zoeos
{

namespace drivers
{
    using namespace hardwareCommunication;

    class RawDataWrapper;

    class AMD_AM79C973 : public Driver, public InterruptRoutine
    {
    private:
        struct Initialization_block
        {
            uint16_t mode;
            unsigned reserved1 : 4;
            unsigned receive_length : 4;
            unsigned reserved2 : 4;
            unsigned transfer_length : 4;
            uint64_t physical_address : 48;
            uint16_t reserved3;
            uint64_t logical_address;
            uint32_t receive_descriptor;
            uint32_t transmit_descriptor;
        } __attribute__((packed));

        struct BufferDescriptor
        {
            uint32_t address;
            uint32_t flags;
            uint32_t flags2;
            uint32_t avail;
        } __attribute__((packed));

    private:
        Port16Bit MACAddress0Port;
        Port16Bit MACAddress2Port;
        Port16Bit MACAddress4Port;

        Port16Bit registerDataPort;
        Port16Bit registerAddressPort;
        Port16Bit resetPort;
        Port16Bit busControlRegisterDataPort;

        Initialization_block initBlock;

        BufferDescriptor *sendBufferDesc;
        uint8_t sendBufferDescMemory[2063];
        uint8_t sendBuffer[2063][8];
        uint8_t currentSendBuffer;
        
        BufferDescriptor *recvBufferDesc;
        uint8_t recvBufferDescMemory[2063];
        uint8_t recvBuffer[2063][8];
        uint8_t currentRecvBuffer;
        RawDataWrapper *wrapper;

    public:
        AMD_AM79C973(PciConfigSpace *device, InterruptManager *interrupts);
        ~AMD_AM79C973();

        virtual void activate() override;
        virtual void deactivate() override { }
        virtual int reset() override;
        virtual uint32_t routine(uint32_t esp) override;
        void send(uint8_t *buffer, int size);
        void receive();
        uint64_t getMACAddr() const;

        void setWrapper(RawDataWrapper *wrapper);
    };

    class RawDataWrapper
    {
    public:
        RawDataWrapper(AMD_AM79C973 *backend_);
        ~RawDataWrapper();

        virtual bool onRawDataReceived(uint8_t *buffer, uint32_t size);
        virtual void send(uint8_t *buffer, uint32_t size);
    protected:
        AMD_AM79C973 *backend;
    };
}

}

#endif