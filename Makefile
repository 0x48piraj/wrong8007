obj-m := wrong8007.o
wrong8007-objs := core.o trigger_keyboard.o # Add other triggers here
KDIR := /usr/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target: build the module
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Load module with runtime params
load:
	@if [ -z "$(PHRASE)" ] || [ -z "$(EXEC)" ]; then \
		echo "Usage: make load PHRASE='<phrase>' EXEC='<path-to-script>'"; \
		exit 1; \
	fi; \
	sudo insmod wrong8007.ko phrase='$(PHRASE)' exec='$(EXEC)'

# Unload the module
remove:
	sudo rmmod wrong8007.ko

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.ko *.mod.* *.o Module.symvers modules.order
