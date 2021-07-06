#include "hardwareCommunication/interrupts.h"

void printf(const char *);

using namespace zoeos;
using namespace zoeos::common;
using namespace zoeos::hardwareCommunication;

InterruptManager::GateDescriptor InterruptManager::IDT[256];
InterruptManager *InterruptManager::activeInterruptManager = nullptr;

InterruptManager::InterruptManager(uint16_t hardwareInterruptOffset_, GlobalDescriptorTable *gdt, TaskManager *taskManager_) : priCommand(0x20), priData(0x21), semiCommand(0xA0), semiData(0xA1)
{
    hardwareInterruptOffset = hardwareInterruptOffset_;
    taskManager = taskManager_;
    const uint8_t __IDT_INTERRUPT_GATE_TYPE_ = 0xe;
    uint16_t codeSegment = (gdt->getCodeSegmentSelector()) << 3;

    for (uint16_t i = 0; i < 256; i++)
    {
        routines[i] = nullptr;
        setGateDescriptor(i, codeSegment, &interruptIgnore, 0, __IDT_INTERRUPT_GATE_TYPE_);
    }

#define XX(name)                                                       \
    setGateDescriptor(0x##name, codeSegment, &HandleException0x##name, \
                      0, __IDT_INTERRUPT_GATE_TYPE_)

    XX(00);
    XX(01);
    XX(02);
    XX(03);
    XX(04);
    XX(05);
    XX(06);
    XX(07);
    XX(08);
    XX(09);
    XX(0A);
    XX(0B);
    XX(0C);
    XX(0D);
    XX(0E);
    XX(0F);
    XX(10);
    XX(11);
    XX(12);
    XX(13);
#undef XX

#define XX(name)                                                       \
    setGateDescriptor(hardwareInterruptOffset + 0x##name, codeSegment, \
                      &HandleInterruptRequest0x##name,                 \
                      0, __IDT_INTERRUPT_GATE_TYPE_)

    XX(00);
    XX(01);
    XX(02);
    XX(03);
    XX(04);
    XX(05);
    XX(06);
    XX(07);
    XX(08);
    XX(09);
    XX(0A);
    XX(0B);
    XX(0C);
    XX(0D);
    XX(0E);
    XX(0F);
    XX(31);
#undef XX

    priCommand.write(0x11);
    semiCommand.write(0x11);
    priData.write(hardwareInterruptOffset);
    semiData.write(hardwareInterruptOffset + 8);
    priData.write(0x04);
    semiData.write(0x02);
    priData.write(0x01);
    semiData.write(0x01);
    priData.write(0x00);
    semiData.write(0x00);

    InterruptDescriptorTablePointer idtr;
    idtr.limit = 256 * sizeof(GateDescriptor) - 1;
    idtr.base = (uint32_t)IDT;

    asm volatile("lidt %0"
                 :
                 : "m"(idtr));
}

InterruptManager::~InterruptManager() {}

void InterruptManager::interruptIgnore()
{
}

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp)
{
    if (activeInterruptManager)
    {
        return activeInterruptManager->handleInt(interruptNumber, esp);
    }
    return esp;
}

void InterruptManager::setGateDescriptor(uint8_t interruptNumber, uint16_t codeSegmentSelector_, void (*handle)(), uint8_t DPL, uint8_t type)
{
    IDT[interruptNumber].lowbits = ((uint32_t)handle) & 0xffff;
    IDT[interruptNumber].highbits = ((uint32_t)handle >> 16) & 0xffff;
    IDT[interruptNumber].codeSegmentSelector = codeSegmentSelector_;
    IDT[interruptNumber].access = 0x80 | ((DPL & 3) << 5) | type;
    IDT[interruptNumber].reserved = 0;
}

void InterruptManager::activate()
{
    if (activeInterruptManager != nullptr)
    {
        activeInterruptManager->deactivate();
    }
    activeInterruptManager = this;
    asm volatile("sti");
}

void InterruptManager::deactivate()
{
    if (activeInterruptManager == this)
    {
        activeInterruptManager = nullptr;
        asm volatile("cli");
    }
}

uint32_t InterruptManager::handleInt(uint8_t interruptNumber, uint32_t esp)
{
    if (routines[interruptNumber])
    {
        esp = routines[interruptNumber]->routine(esp);
    }
    else if (interruptNumber != hardwareInterruptOffset)
    {
        char *msg = (char *)"unprocessed interrupt 0x00\n";
        const char *hex = "0123456789ABCDEF";
        msg[24] = hex[(interruptNumber >> 4) & 0x0f];
        msg[25] = hex[interruptNumber & 0x0f];
        printf(msg);
    }

    if (interruptNumber == hardwareInterruptOffset)
    {
        esp = (uint32_t)taskManager->schedule((CPUState*)esp);
    }
    
    if (interruptNumber >= hardwareInterruptOffset && interruptNumber < hardwareInterruptOffset + 16)
    {
        priCommand.write(0x20);
        if (interruptNumber >= hardwareInterruptOffset + 8)
        {
            semiCommand.write(0x20);
        }
    }
    return esp;
}

InterruptRoutine::InterruptRoutine(uint8_t interruptNumber_, InterruptManager *interruptManager_)
{
    interruptNumber = interruptNumber_;
    interruptManager = interruptManager_;
    interruptManager->routines[interruptNumber] = this;
}

InterruptRoutine::~InterruptRoutine()
{
    if (interruptManager->routines[interruptNumber] == this)
    {
        interruptManager->routines[interruptNumber] = nullptr;
    }
}

uint32_t InterruptRoutine::routine(uint32_t esp)
{
    return esp;
}
