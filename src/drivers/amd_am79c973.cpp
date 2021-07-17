#include "drivers/amd_am79c973.h"

using namespace zoeos;
using namespace zoeos::common;
using namespace zoeos::drivers;
using namespace zoeos::hardwareCommunication;

void printf(const char *);
void printHex(uint8_t);

AMD_AM79C973::AMD_AM79C973(PciConfigSpace *device, InterruptManager *interrupts) : Driver(),
    InterruptRoutine(device->getInterruptNum() + interrupts->getOffset(), interrupts),
    MACAddress0Port(device->getPortBase()),
    MACAddress2Port(device->getPortBase() + 0x20),
    MACAddress4Port(device->getPortBase() + 0x40),
    registerDataPort(device->getPortBase() + 0x10),
    registerAddressPort(device->getPortBase() + 0x12),
    resetPort(device->getPortBase() + 0x14),
    busControlRegisterDataPort(device->getPortBase() + 0x16)
{
    wrapper = nullptr;

    currentSendBuffer = 0;
    currentRecvBuffer = 0;

    uint64_t MAC0 = MACAddress0Port.read() % 256;
    uint64_t MAC1 = MACAddress0Port.read() / 256;
    uint64_t MAC2 = MACAddress2Port.read() % 256;
    uint64_t MAC3 = MACAddress2Port.read() / 256;
    uint64_t MAC4 = MACAddress4Port.read() % 256;
    uint64_t MAC5 = MACAddress4Port.read() / 256;
    uint64_t MAC = MAC5 << 40 | MAC4 << 32 | MAC3 << 24 | MAC2 << 16 | MAC1 << 8 | MAC0;

    registerAddressPort.write(20);
    busControlRegisterDataPort.write(0x102);
    registerAddressPort.write(0);
    registerDataPort.write(0x04);

    initBlock.mode = 0;
    initBlock.reserved1 = 0;
    initBlock.transfer_length = 3;
    initBlock.reserved2 = 0;
    initBlock.receive_length = 3;
    initBlock.physical_address = MAC;
    initBlock.reserved3 = 0;
    initBlock.logical_address = 0;

    sendBufferDesc = (BufferDescriptor *)(((uint32_t)&sendBufferDescMemory[0] + 15) & 0xfff0);
    initBlock.transmit_descriptor = (uint32_t)sendBufferDesc;
    recvBufferDesc = (BufferDescriptor *)(((uint32_t)&recvBufferDescMemory[0] + 15) & 0xfff0);
    initBlock.receive_descriptor = (uint32_t)recvBufferDesc;

    for (uint8_t i = 0; i < 8; i++)
    {
        sendBufferDesc[i].address = ((uint32_t)&sendBuffer[i] + 15) & 0xfff0;
        sendBufferDesc[i].flags = 0xf7ff;
        sendBufferDesc[i].flags2 = 0;
        sendBufferDesc[i].avail = 0;

        recvBufferDesc[i].address = ((uint32_t)&recvBuffer[i] + 15) & 0xfff0;
        recvBufferDesc[i].flags = 0xf7ff | 0x80000000;
        recvBufferDesc[i].flags2 = 0;
        recvBufferDesc[i].avail = 0;
    }

    registerAddressPort.write(1);
    registerDataPort.write((uint32_t)&initBlock);
    registerAddressPort.write(2);
    registerDataPort.write((uint32_t)&initBlock >> 16);
}

void AMD_AM79C973::activate()
{
    registerAddressPort.write(0);
    registerDataPort.write(0x41);

    registerAddressPort.write(4);
    uint32_t tmp = registerDataPort.read();
    registerAddressPort.write(4);
    registerDataPort.write(tmp | 0xc00);

    registerAddressPort.write(0);
    registerDataPort.write(0x42);
}

int AMD_AM79C973::reset()
{
    resetPort.read();
    resetPort.write(0);
    return 10;
}

uint32_t AMD_AM79C973::routine(uint32_t esp)
{
    printf("interrupt from AMD AM79C973: ");
    registerAddressPort.write(0);
    uint32_t tmp = registerDataPort.read();

    if ((tmp & 0x8000) == 0x8000)
        printf("AMD AM79C973 error!\n");
    else if ((tmp & 0x2000) == 0x2000)
        printf("AMD AM79C973 collision error!\n");
    else if ((tmp & 0x1000) == 0x1000)
        printf("AMD AM79C973 missed frame!\n");
    else if ((tmp & 0x0800) == 0x0800)
        printf("AMD AM79C973 memory error!\n");
    else if ((tmp & 0x0400) == 0x0400)
        receive();
    else if ((tmp & 0x0200) == 0x0200)
        printf("AMD AM79C973 data send interrupt!\n");
    registerAddressPort.write(0);
    registerDataPort.write(tmp);

    if ((tmp & 0x0100) == 0x0100)
        printf("AMD AD79C973 init done!\n");
    return esp;
}

void AMD_AM79C973::send(uint8_t *buffer, int size)
{
    int sendDesc = currentSendBuffer;
    currentSendBuffer = (currentSendBuffer + 1) % 8;
    if (size > 1518)
        size = 1518;

    for (uint8_t *src = buffer + size - 1,
                 *dst = (uint8_t *)(sendBufferDesc[sendDesc].address + size - 1);
         src >= buffer; src--, dst--)
        *dst = *src;

    sendBufferDesc[sendDesc].avail = 0;
    sendBufferDesc[sendDesc].flags = 0x8300f000 | ((uint16_t)((-size) & 0xfff));
    sendBufferDesc[sendDesc].flags2 = 0;
    registerAddressPort.write(0);
    registerDataPort.write(0x48);
}

void AMD_AM79C973::receive()
{
    printf("AMD AMD_AM79C973 received\n");
    for (; (recvBufferDesc[currentRecvBuffer].flags & 0x80000000) == 0;
         currentRecvBuffer = (currentRecvBuffer + 1) % 8)
    {
        if (!(recvBufferDesc[currentRecvBuffer].flags & 0x40000000) &&
            (recvBufferDesc[currentRecvBuffer].flags & 0x30000000) == 0x30000000)
        {
            uint32_t size = recvBufferDesc[currentRecvBuffer].flags && 0xfff;
            if (size > 64)
                size -= 4;

            uint8_t *buffer = (uint8_t *)(recvBufferDesc[currentRecvBuffer].address);
            for (int i = 0; i < size; i++)
            {
                printHex(buffer[i]);
                printf(" ");
            }

            if (wrapper)
            {
                if (wrapper->onRawDataReceived(buffer, size))
                {
                    send(buffer, size);
                }
            }
        }
        recvBufferDesc[currentRecvBuffer].flags2 = 0;
        recvBufferDesc[currentRecvBuffer].flags = 0x8000f7ff;
    }
}

void AMD_AM79C973::setWrapper(RawDataWrapper *wrapper_)
{
    wrapper = wrapper_;
}

uint64_t AMD_AM79C973::getMACAddr() const
{
    return initBlock.physical_address;
}

RawDataWrapper::RawDataWrapper(AMD_AM79C973 *backend_)
{
    backend = backend_;
    backend->setWrapper(this);
}

RawDataWrapper::~RawDataWrapper()
{
    backend->setWrapper(nullptr);
}

bool RawDataWrapper::onRawDataReceived(uint8_t *buffer, uint32_t size)
{
    return false;
}

void RawDataWrapper::send(uint8_t *buffer, uint32_t size)
{
    backend->send(buffer, size);
}