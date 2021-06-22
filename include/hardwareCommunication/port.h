#ifndef __HARDWARE_PORT_H__
#define __HARDWARE_PORT_H__

#include "common/types.h"

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
    
    class Port
    {
    protected:
        uint16_t portnumber;
        Port(uint16_t portnumber);
        ~Port();
    };

    class Port8Bit : public Port
    {
    public:
        Port8Bit(uint16_t portnumber);
        ~Port8Bit();
        virtual void write(uint8_t data);
        virtual uint8_t read();
    };

    class Port8BitSlow : public Port8Bit
    {
    public:
        Port8BitSlow(uint16_t portnumber);
        ~Port8BitSlow();
        virtual void write(uint8_t data);
    };

    class Port16Bit : public Port
    {
    public:
        Port16Bit(uint16_t portnumber);
        ~Port16Bit();
        virtual void write(uint16_t data);
        virtual uint16_t read();
    };

    class Port32Bit : public Port
    {
    public:
        Port32Bit(uint16_t portnumber);
        ~Port32Bit();
        virtual void write(uint32_t data);
        virtual uint32_t read();
    };
}

}

#endif