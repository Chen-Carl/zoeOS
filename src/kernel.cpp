#include "common/types.h"
#include "gdt.h"
#include "hardwareCommunication/interrupts.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/driver.h"

using namespace zoeos;
using namespace zoeos::drivers;
using namespace zoeos::hardwareCommunication;

void printf(const char *str)
{
    // screen address
    static uint16_t *VideoMemory = (uint16_t*)0xb8000;
    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i]; i++)
    {
        switch (str[i])
        {
            case '\n':
                y++;
                x = 0;
                break;
            default:
                VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
                x++;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }
        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
            {
                for (x = 0; x < 80; x++)
                    VideoMemory[i] = (VideoMemory[i] & 0xFF00) | ' ';
            }
            x = 0;
            y = 0;
        }
    }
}

void kernelMain(void *multiboot_structure, uint32_t magicnumber)
{
    printf("hello world\n");
    printf("hello myos\n");
    GlobalDescriptorTable gdt;
    DriverManager drvManager;
    InterruptManager interrupts(0x20, &gdt);

    KeyboardDriver keyboard(&interrupts);
    drvManager.addDriver(&keyboard);
    MouseDriver mouse(&interrupts);
    drvManager.addDriver(&mouse);
    drvManager.activeAll();

    interrupts.activate();
    while (1);
}

typedef void (*constructor)();
constructor start_ctors;
constructor end_ctors;

void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
    {
        (*i)();
    }
}