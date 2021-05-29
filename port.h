#ifndef __PORT_H__
#define __PORT_H__

#include "types.h"

template <class T>
class Port
{
public:
    Port(uint16_t portnumber);
    ~Port();
    virtual void write(T data);
    virtual T read();
protected:
    uint16_t m_portnumber;
};

template <class T>
class PortSlow : public Port<T>
{
public:
    PortSlow(uint16_t portnumber);
    ~PortSlow();
    virtual void write(T data);
    virtual T read();
};

#endif