#ifndef __DRIVERS_MOUSE_H__
#define __DRIVERS_MOUSE_H__

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

    class MouseDriver : public InterruptRoutine, public Driver
    {
    public:
        MouseDriver(InterruptManager *manager);
        ~MouseDriver();

        virtual uint32_t routine(uint32_t esp) override;

        // driver methods override
        virtual void activate();

    private:
        uint8_t offset = 0;
        uint8_t buttons = 0;
        uint8_t buffer[3];
        int8_t x, y;

        Port8Bit dataPort;
        Port8Bit commandPort;
    };
}

}


#endif