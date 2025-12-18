obj-m := wrong8007.o
wrong8007-objs := core.o trigger/keyboard.o trigger/usb.o trigger/network.o
KDIR := /usr/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
ccflags-y += -I$(src)/include

# Default target: build the module
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Load module with runtime params
load:
	@if [ -z "$(EXEC)" ]; then \
		echo "Usage: make load EXEC='<path-to-script>' [PHRASE='<phrase>'] [USB_DEVICES='vid:pid:event,...'] [WHITELIST=0|1] [NETWORK PARAMS]"; \
		echo ""; \
		echo "USB params:"; \
		echo "  USB_DEVICES='1234:5678:insert,abcd:ef00:eject,0xXXXX:0xYYYY:any'"; \
		echo "  WHITELIST=1 (only allow listed devices, block others)"; \
		echo "  WHITELIST=0 (block listed devices, allow others)"; \
		echo ""; \
		echo "Network params:"; \
		echo "  MATCH_MAC='aa:bb:cc:dd:ee:ff'"; \
		echo "  MATCH_IP='192.168.1.50'"; \
		echo "  MATCH_PORT=1234"; \
		echo "  MATCH_PAYLOAD='magicstring'"; \
		echo "  HEARTBEAT_HOST='192.168.1.1'"; \
		echo "  HEARTBEAT_INTERVAL=10"; \
		echo "  HEARTBEAT_TIMEOUT=30"; \
		exit 1; \
	fi

	@PARAMS="exec='$(EXEC)'"; \
	[ -n "$(PHRASE)" ] && PARAMS="$$PARAMS phrase=\"$(PHRASE)\""; \
	[ -n "$(USB_DEVICES)" ] && PARAMS="$$PARAMS usb_devices=$(USB_DEVICES)"; \
	[ -n "$(WHITELIST)" ] && PARAMS="$$PARAMS whitelist=$(WHITELIST)"; \
	[ -n "$(MATCH_MAC)" ] && PARAMS="$$PARAMS match_mac=$(MATCH_MAC)"; \
	[ -n "$(MATCH_IP)" ] && PARAMS="$$PARAMS match_ip=$(MATCH_IP)"; \
	[ -n "$(MATCH_PORT)" ] && PARAMS="$$PARAMS match_port=$(MATCH_PORT)"; \
	[ -n "$(MATCH_PAYLOAD)" ] && PARAMS="$$PARAMS match_payload=\"$(MATCH_PAYLOAD)\""; \
	[ -n "$(HEARTBEAT_HOST)" ] && PARAMS="$$PARAMS heartbeat_host=$(HEARTBEAT_HOST)"; \
	[ -n "$(HEARTBEAT_INTERVAL)" ] && PARAMS="$$PARAMS heartbeat_interval=$(HEARTBEAT_INTERVAL)"; \
	[ -n "$(HEARTBEAT_TIMEOUT)" ] && PARAMS="$$PARAMS heartbeat_timeout=$(HEARTBEAT_TIMEOUT)"; \
	echo "sudo insmod wrong8007.ko $$PARAMS"; \
	sudo insmod wrong8007.ko $$PARAMS

# Unload the module
remove:
	sudo rmmod wrong8007.ko

# Reload the module
reload: remove default load

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.ko *.mod.* *.o Module.symvers modules.order
