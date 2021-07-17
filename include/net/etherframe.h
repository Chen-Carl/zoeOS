#ifndef __NET_H__
#define __NET_H__

#include "common/types.h"
#include "drivers/amd_am79c973.h"
#include "memoryManager.h"

namespace zoeos
{

namespace net
{
    struct EtherFrameHeader
    {
        uint64_t dstMAC_BE : 48;
        uint64_t srcMAC_BE : 48;
        uint16_t etherType_BE;
    } __attribute__((packed));

    class EtherFrameHandler;

    class EtherFrameWrapper : public drivers::RawDataWrapper
    {
        friend class EtherFrameHandler;
    public:
        EtherFrameWrapper(drivers::AMD_AM79C973 *backend);
        ~EtherFrameWrapper();

        virtual bool onRawDataReceived(uint8_t *buffer, uint32_t size) override;
        void send(common::uint64_t dstMAC, common::uint16_t etherType, common::uint8_t* buffer, common::uint32_t size);
        
    private:
        EtherFrameHandler *handlers[65536];
    };

    class EtherFrameHandler
    {
    public:
        EtherFrameHandler(EtherFrameWrapper *etherFrameWrapper_, uint16_t etherType_);
        ~EtherFrameHandler();

        bool onEtherFrameReceived(uint8_t *payload, uint32_t size);
        void send(common::uint64_t dstMAC, common::uint8_t* etherframePayload, common::uint32_t size);
    private:
        EtherFrameWrapper* etherFrameWrapper;
        uint16_t etherType;
    };
}

}

#endif