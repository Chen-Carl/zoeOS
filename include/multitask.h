#ifndef __MULTITASK_H__
#define __MULTITASK_H__

#include "common/types.h"
#include "gdt.h"

namespace zoeos
{

using namespace zoeos::common;

struct CPUState
{
    uint32_t eax, ebx, ecx, edx, esi, edi, ebp;
    uint32_t error, eip, cs, eflags, esp, ss;
} __attribute__((packed));

class Task
{
public:
    Task(GlobalDescriptorTable *gdt, void (*entrypoint)());
    ~Task() { }
    void saveState(CPUState *state) { cpuState = state; }
    CPUState *getCpuState() const { return cpuState; }

private:
    uint8_t stack[4096];
    CPUState *cpuState;
};

class TaskManager
{
public:
    TaskManager();
    ~TaskManager();

    bool addTask(Task *task);
    CPUState *schedule(CPUState *cpustate);

private:
    Task *tasks[256];
    int numTasks;
    int currentTask;
};

}

#endif
