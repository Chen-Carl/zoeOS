#include "drivers/driver.h"
#include "common/types.h"

using namespace zoeos::drivers;

Driver::Driver()
{

}

Driver::~Driver()
{

}

void Driver::activate()
{

}

int Driver::reset()
{
    return 0;
}

void Driver::deactivate()
{

}

DriverManager::DriverManager()
{

}

void DriverManager::addDriver(Driver *drv)
{
    drivers[numDrivers++] = drv;
}

void DriverManager::activeAll()
{
    for (uint8_t i = 0; i <numDrivers; i++)
    {
        drivers[i]->activate();
    }
}