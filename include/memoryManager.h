#ifndef __MEMORY_MANAGER_h__
#define __MEMORY_MANAGER_h__

#include "common/types.h"

namespace zoeos
{

using namespace common;

struct MemoryChunk
{
    MemoryChunk *succ;
    MemoryChunk *pred;
    bool allocated;
    size_t size;
};

class MemoryManager
{
public:
    MemoryManager(size_t start, size_t size);
    ~MemoryManager();

    void *malloc(size_t size);
    void free (void *ptr);
    static MemoryManager *activeMM;

private:
    MemoryChunk *first;
};

}

using zoeos::common::size_t;

void *operator new(size_t size);
void *operator new[](size_t size);
void *operator new(size_t size, void *ptr);
void *operator new[](size_t size, void *ptr);

void operator delete(void *ptr);
void operator delete[](void *ptr);

#endif
