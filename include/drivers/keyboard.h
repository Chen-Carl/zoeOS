#ifndef __DRIVERS_KEYBOARD_H__
#define __DRIVERS_KEYBOARD_H__

#include "common/types.h"
#include "hardwareCommunication/interrupts.h"
#include "hardwareCommunication/port.h"
#include "drivers/driver.h"

namespace zoeos
{

namespace drivers
{
    using hardwareCommunication::InterruptManager;
    using hardwareCommunication::InterruptRoutine;
    using hardwareCommunication::Port8Bit;

    class KeyboardDriver : public InterruptRoutine, public Driver
    {
    public:
        KeyboardDriver(InterruptManager *manager);
        ~KeyboardDriver();

        virtual uint32_t routine(uint32_t esp) override;

        // driver methods override
        virtual void activate();

    private:
        Port8Bit dataPort;
        Port8Bit commandPort;
    };
}
}

#endif