#include "mouse.h"

void printf(const char *);

MouseDriver::MouseDriver(InterruptManager *manager)
    : InterruptRoutine(0x0C + manager->getOffset(), manager),
      dataPort(0x60),
      commandPort(0x64),
      x(40), y(12)
{
    uint16_t* videoMemory = (uint16_t*)0xb8000;
    videoMemory[y * 80 + x] = ((videoMemory[y * 80 + x] & 0xf000) >> 4) | 
                              ((videoMemory[y * 80 + x] & 0x0f00) << 4) |
                              (videoMemory[y * 80 + x] & 0x00ff);
    commandPort.write(0xa8);
    commandPort.write(0x20);
    uint8_t status = (dataPort.read() | 0x02) & (~0x20);
    commandPort.write(0x60);
    dataPort.write(status);
    commandPort.write(0xd4);
    dataPort.write(0xf4);
    dataPort.read();
}

MouseDriver::~MouseDriver()
{
}

uint32_t MouseDriver::routine(uint32_t esp)
{
    uint8_t status = commandPort.read();
    if (!(status & 0x20))
        return esp;

    buffer[offset] = dataPort.read();
    offset = (offset + 1) % 3;
    if (offset == 0)
    {
        uint16_t* videoMemory = (uint16_t*)0xb8000;
        videoMemory[y * 80 + x] = ((videoMemory[y * 80 + x] & 0xf000) >> 4) | 
                                  ((videoMemory[y * 80 + x] & 0x0f00) << 4) |
                                  (videoMemory[y * 80 + x] & 0x00ff);
        x += buffer[1];
        x = (x < 0 ? 0 : x);
        x = (x >= 80 ? 79 : x);

        y -= buffer[2];
        y = (y < 0 ? 0 : y);
        y = (y >= 25 ? 24 : y);

        videoMemory[y * 80 + x] = ((videoMemory[y * 80 + x] & 0xf000) >> 4) | 
                                  ((videoMemory[y * 80 + x] & 0x0f00) << 4) |
                                  (videoMemory[y * 80 + x] & 0x00ff);
        
        for (uint8_t i = 0; i < 3; i++)
        {
            if ((buffer[0] & (1 << i)) != (buttons & (1 << i)))
            {
                videoMemory[y * 80 + x] = ((videoMemory[y * 80 + x] & 0xf000) >> 4) | 
                ((videoMemory[y * 80 + x] & 0x0f00) << 4) |
                (videoMemory[y * 80 + x] & 0x00ff);
            }
        }
        buttons = buffer[0];
    }
    return esp;
}
