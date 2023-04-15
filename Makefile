obj-m := wrong8007.o
KDIR := /usr/lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) M=$(PWD) modules
remove:
	sudo rmmod -f wrong8007.ko
clean:
	rm -r -f .wrong8007.* wrong8007.ko wrong8007.mod.c wrong8007.mod.o wrong8007.o Module.symvers modules.order .tmp_versions
