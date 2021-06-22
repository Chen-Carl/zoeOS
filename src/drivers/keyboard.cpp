#include "drivers/keyboard.h"

void printf(const char *);

using namespace zoeos::drivers;
using namespace zoeos::common;
using namespace zoeos::hardwareCommunication;

KeyboardDriver::KeyboardDriver(InterruptManager *manager)
    : InterruptRoutine(0x01 + manager->getOffset(), manager),
      dataPort(0x60),
      commandPort(0x64)
{
    
}

KeyboardDriver::~KeyboardDriver()
{
}

void KeyboardDriver::activate()
{
    while (commandPort.read() & 0x1)
    {
        dataPort.read();
    }
    commandPort.write(0xae);
    commandPort.write(0x20);
    uint8_t status = (dataPort.read() | 0x01) & (~0x10);
    commandPort.write(0x60);
    dataPort.write(status);
    dataPort.write(0xf4);
}

uint32_t KeyboardDriver::routine(uint32_t esp)
{
    uint8_t key = dataPort.read();

    // press shift to input uppercase letter
    static bool shift = false;
    switch (key)
    {
#define XX(num, upper, lower) \
    case 0x##num:             \
        if (shift)            \
            printf(#upper);   \
        else                  \
            printf(#lower);   \
        break

        XX(02, !, 1);
        XX(03, @, 2);
        XX(04, #, 3);
        XX(05, $, 4);
        XX(06, %, 5);
        XX(07, ^, 6);
        XX(08, &, 7);
        XX(09, *, 8);
        XX(0A, (, 9);
        XX(0B, ), 0);

        XX(10, Q, q);
        XX(11, W, w);
        XX(12, E, e);
        XX(13, R, r);
        XX(14, T, t);
        XX(15, Y, y);
        XX(16, U, u);
        XX(17, I, i);
        XX(18, O, o);
        XX(19, P, p);

        XX(1E, A, a);
        XX(1F, S, s);
        XX(20, D, d);
        XX(21, F, f);
        XX(22, G, g);
        XX(23, H, h);
        XX(24, J, j);
        XX(25, K, k);
        XX(26, L, l);

        XX(2C, Y, y);
        XX(2D, X, x);
        XX(2E, C, c);
        XX(2F, V, v);
        XX(30, B, b);
        XX(31, N, n);
        XX(32, M, m);
    case 0x33:
        if (shift)
            printf("<");
        else
            printf(",");
        break;
        XX(34, >, .);
        XX(35, ?, /);
#undef XX

    case 0x1C:
        printf("\n");
        break;
    case 0x39:
        printf(" ");
        break;
    case 0x2A:
    case 0x36:
        shift = true;
        break;
    case 0xAA:
    case 0xB6:
        shift = false;
        break;

    case 0x45:
        break;
    default:
        if (key < 0x80)
        {
            char *msg = (char *)"keyboard 0x00\n";
            const char *hex = "0123456789ABCDEF";
            msg[11] = hex[(key >> 4) & 0x0f];
            msg[12] = hex[key & 0x0f];
            printf(msg);
        }
    }

    return esp;
}
