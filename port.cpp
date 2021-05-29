#include "port.h"

template <class T>
Port<T>::Port(uint16_t portnumber)
    : m_portnumber(portnumber) { }

template <class T>
Port<T>::~Port() { }

template <class T>
void Port<T>::write(T data)
{

}

template <class T>
T Port<T>::read()
{

}

template <>
void Port<uint8_t>::write(uint8_t data)
{
    asm volatile("outb %0, %1"
                 : "=a" (data)
                 : "Nd" (m_portnumber)                
    );
}

template <>
uint8_t Port<uint8_t>::read()
{
    uint8_t result;
    asm volatile("inb %1, %0"
                 : "=a" (result)
                 : "Nd" (m_portnumber)
    );
    return result;
}

template <>
void Port<uint16_t>::write(uint16_t data)
{
    asm volatile("outw %0, %1"
                 : "=a" (data)
                 : "Nd" (m_portnumber)                
    );
}

template <>
uint16_t Port<uint16_t>::read()
{
    uint16_t result;
    asm volatile("inw %1, %0"
                 : "=a" (result)
                 : "Nd" (m_portnumber)
    );
    return result;
}

template <>
void Port<uint32_t>::write(uint32_t data)
{
    asm volatile("outl %0, %1"
                 : "=a" (data)
                 : "Nd" (m_portnumber)                
    );
}

template <>
uint32_t Port<uint32_t>::read()
{
    uint32_t result;
    asm volatile("inl %1, %0"
                 : "=a" (result)
                 : "Nd" (m_portnumber)
    );
    return result;
}

template <class T>
PortSlow<T>::PortSlow(uint16_t portnumber) : Port<T>(portnumber)
{

}

template <class T>
PortSlow<T>::~PortSlow()
{

}

template <class T>
void PortSlow<T>::write(T data)
{

}

template <class T>
T PortSlow<T>::read()
{

}

template <>
void PortSlow<uint8_t>::write(uint8_t data)
{
    asm volatile("outb %0, %1\n"
                 "jmp 1f\n"
                 "1: jmp 1f\n"
                 "1:\n"
                 : "=a" (data)
                 : "Nd" (m_portnumber)
    );
}