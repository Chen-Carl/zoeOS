#include "net/etherframe.h"

using namespace zoeos;
using namespace zoeos::common;
using namespace zoeos::net;
using namespace zoeos::drivers;

EtherFrameWrapper::EtherFrameWrapper(drivers::AMD_AM79C973 *backend)
    : RawDataWrapper(backend)
{
    for (uint32_t i = 0; i < 65536; i++)
    {
        handlers[i] = nullptr;
    }
}

EtherFrameWrapper::~EtherFrameWrapper() { }

bool EtherFrameWrapper::onRawDataReceived(uint8_t *buffer, uint32_t size)
{
    EtherFrameHeader *frame = (EtherFrameHeader*)buffer;
    bool sendBack = false;
    if ((frame->dstMAC_BE == 0xffffffffffff) || (frame->dstMAC_BE == backend->getMACAddr()))
    {
        if (handlers[frame->etherType_BE] != 0)
        {
            sendBack = handlers[frame->etherType_BE]->onEtherFrameReceived(buffer + sizeof(EtherFrameHeader), size - sizeof(EtherFrameHeader));
        }
    }

    if (sendBack)
    {
        frame->dstMAC_BE = frame->srcMAC_BE;
        frame->srcMAC_BE = backend->getMACAddr();
    }
    return sendBack;
}

void EtherFrameWrapper::send(uint64_t dstMAC, uint16_t etherType, common::uint8_t* buffer, common::uint32_t size)
{
    uint8_t *frame = (uint8_t*)MemoryManager::activeMM->malloc(sizeof(EtherFrameHeader) + size);
    EtherFrameHeader *frameHeader = (EtherFrameHeader*)frame;
    frameHeader->dstMAC_BE = dstMAC;
    frameHeader->srcMAC_BE = backend->getMACAddr();
    frameHeader->etherType_BE = etherType;

    uint8_t *src = buffer;
    uint8_t *dst = frame + sizeof(EtherFrameHeader);
    for (uint32_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }
    backend->send(frame, size + sizeof(EtherFrameHeader));
}

EtherFrameHandler::EtherFrameHandler(EtherFrameWrapper *etherFrameWrapper_, uint16_t etherType_)
{
    etherType = ((etherType_ & 0x00ff) << 8) | ((etherType_ & 0xff00) >> 8);
    etherFrameWrapper = etherFrameWrapper_;
    etherFrameWrapper->handlers[etherType] = this;
}

EtherFrameHandler::~EtherFrameHandler()
{
    etherFrameWrapper->handlers[etherType] = nullptr;
}

bool EtherFrameHandler::onEtherFrameReceived(uint8_t *payload, uint32_t size)
{
    return false;
}

void EtherFrameHandler::send(common::uint64_t dstMAC, common::uint8_t* etherframePayload, common::uint32_t size)
{
    etherFrameWrapper->send(dstMAC, etherType, etherframePayload, size);
}