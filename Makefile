obj-m := wrong8007.o
wrong8007-objs := core.o trigger_keyboard.o trigger_usb.o
KDIR := /usr/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target: build the module
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Load module with runtime params
load:
	@if [ -z "$(PHRASE)" ] || [ -z "$(EXEC)" ] || [ -z "$(USB_VID)" ] || [ -z "$(USB_PID)" ]; then \
		echo "Usage: make load PHRASE='<phrase>' EXEC='<path-to-script>' USB_VID=0xXXXX USB_PID=0xYYYY [USB_EVENT=insert|eject|any]"; \
		exit 1; \
	fi

	sudo insmod wrong8007.ko \
		phrase='$(PHRASE)' \
		exec='$(EXEC)' \
		usb_vid=$(USB_VID) \
		usb_pid=$(USB_PID) \
		$(if $(USB_EVENT),usb_event=$(USB_EVENT))

# Unload the module
remove:
	sudo rmmod wrong8007.ko

# Reload the module
reload: remove default load

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.ko *.mod.* *.o Module.symvers modules.order
