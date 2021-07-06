#include "memoryManager.h"

using namespace zoeos;
using namespace zoeos::common;

MemoryManager *MemoryManager::activeMM = nullptr;

MemoryManager::MemoryManager(size_t start, size_t size)
{
    activeMM = this;
    if (size < sizeof(MemoryChunk))
    {
        first = nullptr;
    }
    else
    {
        first = (MemoryChunk*)start;
        first->allocated = false;
        first->pred = nullptr;
        first->succ = nullptr;
        first->size = size - sizeof(MemoryChunk);
    }
}

MemoryManager::~MemoryManager() 
{
    if (activeMM == this)
    {
        activeMM = nullptr;
    }
}

void *MemoryManager::malloc(size_t size)
{
    MemoryChunk *result = nullptr;
    for (MemoryChunk *chunk = first; chunk != nullptr && result == nullptr; chunk = chunk->succ)
    {
        if (chunk->size > size && !chunk->allocated)
        {
            result = chunk;
        }
    }
    if (result == nullptr)
        return nullptr;
    if (result->size > size + sizeof(MemoryChunk))
    {
        MemoryChunk *tmp = (MemoryChunk*)((size_t)result + sizeof(MemoryChunk) + size);
        tmp->allocated = false;
        tmp->size = result->size - size - sizeof(MemoryChunk);
        tmp->pred = result;
        tmp->succ = result->succ;
        if (tmp->succ != nullptr)
        {
            tmp->succ->pred = tmp;
        }
        result->succ = tmp;
        result->size = size;
    } 
    result->allocated = true;
    return (void*)((size_t)result + sizeof(MemoryChunk));
}

void MemoryManager::free(void *ptr)
{
    MemoryChunk *chunk = (MemoryChunk*)((size_t)ptr - sizeof(MemoryChunk));
    chunk->allocated = false;
    if (chunk->pred != nullptr && !chunk->pred->allocated)
    {
        chunk->pred->succ = chunk->succ;
        chunk->pred->size += chunk->size + sizeof(MemoryChunk);
        if (chunk->succ != nullptr)
        {
            chunk->succ->pred = chunk->pred;
        }
        chunk = chunk->pred;
    }

    if (chunk->succ != nullptr && !chunk->succ->allocated)
    {
        chunk->size += chunk->succ->size + sizeof(MemoryChunk);
        chunk->succ = chunk->succ->succ;
        if (chunk->succ != nullptr)
        {
            chunk->succ->pred = chunk;
        }
    }
}

void *operator new(size_t size)
{
    if (MemoryManager::activeMM == nullptr)
        return nullptr;
    return MemoryManager::activeMM->malloc(size);
}

void *operator new[](size_t size)
{
    if (MemoryManager::activeMM == nullptr)
        return nullptr;
    return MemoryManager::activeMM->malloc(size);
}

void *operator new(size_t szie, void *ptr)
{
    return ptr;
}

void *operator new[](size_t size, void *ptr)
{
    return ptr;
}

void operator delete(void *ptr)
{
    if (MemoryManager::activeMM != nullptr)
    {
        MemoryManager::activeMM->free(ptr);
    }
}

void operator delete[](void *ptr)
{
    if (MemoryManager::activeMM != nullptr)
    {
        MemoryManager::activeMM->free(ptr);
    }
}
