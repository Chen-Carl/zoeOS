#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "types.h"
#include "interrupts.h"
#include "port.h"

class KeyboardDriver : public InterruptRoutine
{
public:
    KeyboardDriver(InterruptManager *manager);
    ~KeyboardDriver();

    virtual uint32_t routine(uint32_t esp) override;

private:
    Port8Bit dataPort;
    Port8Bit commandPort;
};

#endif