#ifndef __DRIVERS_DRIVER_H__
#define __DRIVERS_DRIVER_H__

namespace zoeos
{
namespace drivers
{

    class Driver
    {
    public:
        Driver();
        ~Driver();

        virtual void activate();
        virtual int reset();
        virtual void deactivate();
    };

    class DriverManager
    {
    public:
        DriverManager();
        ~DriverManager();

        void addDriver(Driver *);
        void activeAll();
        Driver *drivers[256];

    private:
        int numDrivers = 0;
    };
}
}

#endif
