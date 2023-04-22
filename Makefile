obj-m := wrong8007.o
KDIR := /usr/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) M=$(PWD) modules

load:
	@if [ -z "$(PHRASE)" ] || [ -z "$(EXEC)" ]; then \
		echo "Usage: make load PHRASE='<phrase>' EXEC='<path-to-script>'"; \
		exit 1; \
	fi
	sudo insmod wrong8007.ko phrase='$(PHRASE)' exec='$(EXEC)'

remove:
	sudo rmmod wrong8007.ko

clean:
	rm -r -f .wrong8007.* wrong8007.ko wrong8007.mod.c wrong8007.mod.o wrong8007.o Module.symvers modules.order .tmp_versions
