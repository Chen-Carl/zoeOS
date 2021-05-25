void printf(char *str)
{
    // screen address
    unsigned short *VideoMemory = (unsigned short*)0xb8000;
    for (int i = 0; str[i]; i++)
    {
        VideoMemory[i] = (VideoMemory[i] & 0xFF00) | str[i];
    }
}

void kernelMain(void *multiboot_structure, unsigned int magicnumber)
{
    printf((char*)"hello world");
    while (1);
}

typedef void (*constructor)();
constructor start_ctors;
constructor end_ctors;

void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
    {
        (*i)();
    }
}