#ifndef __HARDWARE_INTERRUPTS_H__
#define __HARDWARE_INTERRUPTS_H__

#include "common/types.h"
#include "port.h"
#include "gdt.h"

namespace zoeos
{

namespace hardwareCommunication
{
    using common::uint8_t;
    using common::uint16_t;
    using common::uint32_t;
    using common::uint64_t;
    using common::int8_t;
    using common::int16_t;
    using common::int32_t;
    using common::int64_t;
    
    class InterruptRoutine;

    class InterruptManager
    {
        friend class InterruptRoutine;
    public:
        InterruptManager(uint16_t hardwareInterruptOffset_, GlobalDescriptorTable *gdt);
        ~InterruptManager();
        void activate();
        void deactivate();
        uint16_t getOffset() const { return hardwareInterruptOffset; }

    private:
        struct GateDescriptor
        {
            uint16_t lowbits;
            uint16_t codeSegmentSelector;
            uint8_t reserved;
            uint8_t access;
            uint16_t highbits;
        } __attribute__((packed));

        struct InterruptDescriptorTablePointer
        {
            uint16_t limit;
            uint32_t base;
        } __attribute__((packed));

    private:
        static void interruptIgnore();

        static uint32_t handleInterrupt(uint8_t interruptNumber, uint32_t esp);

        static void setGateDescriptor(uint8_t interruptNumber, uint16_t codeSegmentSelector, void (*handle)(), uint8_t DPL, uint8_t type);

        static void HandleInterruptRequest0x00();
        static void HandleInterruptRequest0x01();
        static void HandleInterruptRequest0x02();
        static void HandleInterruptRequest0x03();
        static void HandleInterruptRequest0x04();
        static void HandleInterruptRequest0x05();
        static void HandleInterruptRequest0x06();
        static void HandleInterruptRequest0x07();
        static void HandleInterruptRequest0x08();
        static void HandleInterruptRequest0x09();
        static void HandleInterruptRequest0x0A();
        static void HandleInterruptRequest0x0B();
        static void HandleInterruptRequest0x0C();
        static void HandleInterruptRequest0x0D();
        static void HandleInterruptRequest0x0E();
        static void HandleInterruptRequest0x0F();
        static void HandleInterruptRequest0x31();

        static void HandleException0x00();
        static void HandleException0x01();
        static void HandleException0x02();
        static void HandleException0x03();
        static void HandleException0x04();
        static void HandleException0x05();
        static void HandleException0x06();
        static void HandleException0x07();
        static void HandleException0x08();
        static void HandleException0x09();
        static void HandleException0x0A();
        static void HandleException0x0B();
        static void HandleException0x0C();
        static void HandleException0x0D();
        static void HandleException0x0E();
        static void HandleException0x0F();
        static void HandleException0x10();
        static void HandleException0x11();
        static void HandleException0x12();
        static void HandleException0x13();
        
        static InterruptManager *activeInterruptManager;

        uint32_t handleInt(uint8_t interruptNumber, uint32_t esp);

        uint16_t hardwareInterruptOffset;
        static GateDescriptor IDT[256];
        InterruptRoutine* routines[256];

        Port8BitSlow priCommand;
        Port8BitSlow priData;
        Port8BitSlow semiCommand;
        Port8BitSlow semiData;
    };

    class InterruptRoutine
    {
    public:
        virtual uint32_t routine(uint32_t esp);
        InterruptRoutine(uint8_t interruptNumber, InterruptManager* interruptManager);
        ~InterruptRoutine();

    protected:
        uint8_t interruptNumber;
        InterruptManager* interruptManager;
    };
}

}


#endif