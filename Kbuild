# Build profile in Kconfig .config format
WRONG8007_PROFILE ?= $(firstword $(wildcard $(src)/.config) $(src)/configs/defconfig)
include $(WRONG8007_PROFILE)

# Triggers whose kernel subsystem is missing are left out
ifeq ($(CONFIG_WRONG8007_KEYBOARD),y)
ifneq ($(CONFIG_VT),y)
$(warning wrong8007: keyboard trigger left out, the kernel has no CONFIG_VT)
CONFIG_WRONG8007_KEYBOARD :=
endif
endif
ifeq ($(CONFIG_WRONG8007_USB),y)
ifeq ($(filter y m,$(CONFIG_USB)),)
$(warning wrong8007: USB trigger left out, the kernel has no CONFIG_USB)
CONFIG_WRONG8007_USB :=
CONFIG_WRONG8007_WHITELIST :=
endif
endif
ifeq ($(CONFIG_WRONG8007_NETWORK),y)
ifneq ($(CONFIG_NETFILTER),y)
$(warning wrong8007: network trigger left out, the kernel has no CONFIG_NETFILTER)
CONFIG_WRONG8007_NETWORK :=
endif
endif

WRONG8007_VERSION := $(shell sed -n 's/^PACKAGE_VERSION="\(.*\)"$$/\1/p' $(src)/dkms.conf 2>/dev/null)

# External module builds link obj-m
obj-m += $(if $(filter y m,$(CONFIG_WRONG8007)),wrong8007.o)

wrong8007-objs := core.o lib/phrase_match.o lib/parse.o

wrong8007-$(CONFIG_WRONG8007_KEYBOARD) += trigger/keyboard.o
wrong8007-$(CONFIG_WRONG8007_USB) += trigger/usb.o
wrong8007-$(CONFIG_WRONG8007_NETWORK) += trigger/network.o

ccflags-y += -I$(src)/include
ccflags-y += -DWRONG8007_VERSION=\"$(WRONG8007_VERSION)\"

ccflags-$(CONFIG_WRONG8007_KEYBOARD) += -DCONFIG_WRONG8007_KEYBOARD=1
ccflags-$(CONFIG_WRONG8007_USB) += -DCONFIG_WRONG8007_USB=1
ccflags-$(CONFIG_WRONG8007_WHITELIST) += -DCONFIG_WRONG8007_WHITELIST=1
ccflags-$(CONFIG_WRONG8007_NETWORK) += -DCONFIG_WRONG8007_NETWORK=1
ccflags-$(CONFIG_WRONG8007_DEBUG) += -DDEBUG

# Optional additional compiler flags
ccflags-y += $(WB_EXTRA_CFLAGS)
