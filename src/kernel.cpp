#include "common/types.h"
#include "gdt.h"
#include "hardwareCommunication/interrupts.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/driver.h"
#include "hardwareCommunication/pci.h"
#include "multitask.h"
#include "memoryManager.h"

using namespace zoeos;
using namespace zoeos::drivers;
using namespace zoeos::hardwareCommunication;

void printf(const char *str);
void printHex(uint8_t);
void fTask1();
void fTask2();

// screen address
static uint16_t *VideoMemory = (uint16_t*)0xb8000;
static uint8_t x = 0, y = 0;

void printHex(uint8_t n) {
    char* str = (char*)"00";
    const char* hex = "0123456789ABCDEF";
    str[0] = hex[(n >> 4) & 0x0f];
    str[1] = hex[n & 0x0f];
    printf((const char*)str);
}

void printf(const char *str)
{
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

void fTask1()
{
    while (1)
    {
        printf("A");
    }
}

void fTask2()
{
    while (1)
    {
        printf("B");
    }
}

void kernelMain(void *multiboot_structure, uint32_t magicnumber)
{
    printf("hello world\n");
    printf("hello myos\n");
    GlobalDescriptorTable gdt;

    size_t heap = 10 * 1024 * 1024;
    uint32_t *memupper = (uint32_t*)((size_t)multiboot_structure + 8);
    MemoryManager memoryManager(heap, (*memupper) * 1024 - heap - 10 * 1024);

    TaskManager taskManager;
    // Task task1(&gdt, fTask1);
    // Task task2(&gdt, fTask2);
    // taskManager.addTask(&task1);
    // taskManager.addTask(&task2);

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    DriverManager drvManager;

    KeyboardDriver keyboard(&interrupts);
    drvManager.addDriver(&keyboard);

    MouseDriver mouse(&interrupts);
    drvManager.addDriver(&mouse);

    PciController PCI;
    PCI.checkBuses(&drvManager, &interrupts);
    drvManager.activeAll();
    interrupts.activate();

    printf("------------- test allocate --------------\n");
    printHex((heap >> 24) & 0xff);
    printHex((heap >> 16) & 0xff);
    printHex((heap >> 8) & 0xff);
    printHex(heap & 0xff);
    void *allocated = memoryManager.malloc(1024);
    printf("\n allocated: 0x");
    printHex(((size_t)allocated >> 24) & 0xff);
    printHex(((size_t)allocated >> 16) & 0xff);
    printHex(((size_t)allocated >> 8) & 0xff);
    printHex((size_t)allocated & 0xff);
    printf("\n------------- end test allocate --------------\n");

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