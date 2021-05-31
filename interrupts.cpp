#include "interrupts.h"

void printf(const char*);

InterruptManager::GateDescriptor InterruptManager::IDT[256];

InterruptManager::InterruptManager(uint16_t hardwareInterruptOffset_, GlobalDescriptorTable *gdt) : priCommand(0x20), priData(0x21), semiCommand(0xA0), semiData(0xA1)
{
    hardwareInterruptOffset = hardwareInterruptOffset_;
    const uint8_t __IDT_INTERRUPT_GATE_TYPE_ = 0xe;
    uint16_t codeSegment = (gdt->getCodeSegmentSelector()) << 3;

    for (uint16_t i = 0; i < 256; i++)
    {
        setGateDescriptor(i, codeSegment, &interruptIgnore, 0, __IDT_INTERRUPT_GATE_TYPE_);
    }

#define XX(name) \
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

#define XX(name) \
    setGateDescriptor(hardwareInterruptOffset + 0x##name, codeSegment, \
                      &HandleInterruptRequest0x##name, \
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

uint32_t InterruptManager::handleInterrupt(uint8_t interruptNumber, uint32_t esp) 
{ 
    printf("interrupt");
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
    asm volatile("sti");
}
