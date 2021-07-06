#include "multitask.h"

using namespace zoeos::common;
using namespace zoeos;

Task::Task(GlobalDescriptorTable *gdt, void (*entrypoint)())
{
    cpuState = (CPUState*)(stack + 4096 - sizeof(CPUState));
    cpuState->eax = 0;
    cpuState->ebx = 0;
    cpuState->ecx = 0;
    cpuState->edx = 0;

    cpuState->esi = 0;
    cpuState->edi = 0;

    cpuState->ebp = 0;
    cpuState->eip = (uint32_t)entrypoint;
    cpuState->cs = gdt->getCodeSegmentSelector() << 3;
    cpuState->eflags = 0x202;
}

TaskManager::TaskManager() : numTasks(0), currentTask(-1) { }

bool TaskManager::addTask(Task *task)
{
    if (numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    return true;
}

CPUState *TaskManager::schedule(CPUState *cpuState)
{
    if (numTasks <= 0)
        return cpuState;
    
    tasks[currentTask]->saveState(cpuState);
    if (++currentTask >= numTasks)
        currentTask %= numTasks;
    return tasks[currentTask]->getCpuState();
}

