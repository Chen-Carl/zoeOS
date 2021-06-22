#ifndef __GDT_H__
#define __GDT_H__

#include "common/types.h"

namespace zoeos
{
    class GlobalDescriptorTable
    {
    public:
        class SegmentDescriptor
        {
        public:
            // base: base address
            // limit: max segment length
            // type: segment type, eg. data segment, code segment etc.
            SegmentDescriptor(uint32_t base, uint32_t limit, uint8_t type);
            uint32_t Base();
            uint32_t Limit();

        private:
            uint16_t limit_lo;          // limit字段低位
            uint16_t base_lo;           // base字段低位
            uint8_t base_hi;            // base字段次高位
            uint8_t type;               // type字段和其他标志
            uint8_t flags_limit_hi;     // limit字段高位和其他标志
            uint8_t base_vhi;           // base字段最高位
        } __attribute__((packed));

        SegmentDescriptor nullSegmentDescriptor;
        SegmentDescriptor unusedSegmentDescriptor;
        SegmentDescriptor codeSegmentDescriptor;
        SegmentDescriptor dataSegmentDescriptor;

        GlobalDescriptorTable();
        ~GlobalDescriptorTable();

        uint16_t getCodeSegmentSelector();
        uint16_t getDataSegmentSelector();
    };
}

#endif
