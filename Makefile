obj-m := wrong8007.o
wrong8007-objs := core.o trigger_keyboard.o trigger_usb.o trigger_network.o
KDIR := /usr/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Default target: build the module
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Load module with runtime params
load:
	@if [ -z "$(EXEC)" ]; then \
		echo "Usage: make load [PHRASE='<phrase>'] EXEC='<path-to-script>' [USB_VID=0xXXXX USB_PID=0xYYYY [USB_EVENT=insert|eject|any]] [NETWORK PARAMS]"; \
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

	@if [ -n "$(USB_EVENT)" ] && [ "$(USB_EVENT)" != "insert" ] && [ "$(USB_EVENT)" != "eject" ] && [ "$(USB_EVENT)" != "any" ]; then \
		echo "Error: USB_EVENT must be one of: insert, eject, any"; \
		exit 1; \
	fi

	@PARAMS="exec='$(EXEC)'"; \
	[ -n "$(PHRASE)" ] && PARAMS="$$PARAMS phrase=$(PHRASE)"; \
	[ -n "$(USB_VID)" ] && PARAMS="$$PARAMS usb_vid=$(USB_VID)"; \
	[ -n "$(USB_PID)" ] && PARAMS="$$PARAMS usb_pid=$(USB_PID)"; \
	[ -n "$(USB_EVENT)" ] && PARAMS="$$PARAMS usb_event=$(USB_EVENT)"; \
	[ -n "$(MATCH_MAC)" ] && PARAMS="$$PARAMS match_mac=$(MATCH_MAC)"; \
	[ -n "$(MATCH_IP)" ] && PARAMS="$$PARAMS match_ip=$(MATCH_IP)"; \
	[ -n "$(MATCH_PORT)" ] && PARAMS="$$PARAMS match_port=$(MATCH_PORT)"; \
	[ -n "$(MATCH_PAYLOAD)" ] && PARAMS="$$PARAMS match_payload='$(MATCH_PAYLOAD)'"; \
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
