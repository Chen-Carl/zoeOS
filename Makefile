GPPPARAMS = -m32 -fno-use-cxa-atexit -fleading-underscore -fno-exceptions -fno-builtin -nostdlib -fno-rtti -fno-pie -Iinclude
ASPARAMS = --32
LDPARAMS = -melf_i386 -no-pie

# object files
objects = obj/loader.o \
		  obj/kernel.o \
		  obj/gdt.o \
		  obj/memoryManager.o \
		  obj/multitask.o \
		  obj/hardwareCommunication/port.o  \
		  obj/hardwareCommunication/interrupts.o \
		  obj/hardwareCommunication/asm_interrupts.o \
		  obj/hardwareCommunication/pci.o \
		  obj/drivers/keyboard.o \
		  obj/drivers/mouse.o \
		  obj/drivers/driver.o \
		  obj/drivers/amd_am79c973.o \
		  obj/net/etherframe.o

obj/%.o: src/%.cpp
	mkdir -p $(@D)
	g++ ${GPPPARAMS} -o $@ -c $<

obj/%.o: src/%.s
	mkdir -p $(@D)	
	as ${ASPARAMS} -o $@ $<

mykernel.bin: linker.ld ${objects}
	mkdir -p $(@D)
	ld ${LDPARAMS} -T $< -o $@ ${objects}

# install: mykernel.bin
# 	sudo cp $< /boot/mykernel.bin

mykernel.iso: mykernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp $< iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "my os" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/mykernel.bin' >> iso/boot/grub/grub.cfg
	echo '    boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=$@ iso
	rm -rf iso

.PHONY: clean
clean:
	rm -rf obj
	rm mykernel.bin mykernel.iso