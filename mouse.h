#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "types.h"
#include "interrupts.h"
#include "port.h"

class MouseDriver : public InterruptRoutine
{
public:
    MouseDriver(InterruptManager *manager);
    ~MouseDriver();

    virtual uint32_t routine(uint32_t esp) override;

private:
    uint8_t offset = 0;
    uint8_t buttons = 0;
    uint8_t buffer[3];
    int8_t x, y;

    Port8Bit dataPort;
    Port8Bit commandPort;
};

#endif