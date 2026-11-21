# Build profile in Kconfig .config format
WRONG8007_PROFILE ?= $(firstword $(wildcard $(src)/.config) $(src)/configs/defconfig)
include $(WRONG8007_PROFILE)

# External module builds link obj-m
obj-m += $(if $(filter y m,$(CONFIG_WRONG8007)),wrong8007.o)

wrong8007-objs := core.o lib/phrase_match.o lib/parse.o

wrong8007-$(CONFIG_WRONG8007_KEYBOARD) += trigger/keyboard.o
wrong8007-$(CONFIG_WRONG8007_USB) += trigger/usb.o
wrong8007-$(CONFIG_WRONG8007_NETWORK) += trigger/network.o

ccflags-y += -I$(src)/include

ccflags-$(CONFIG_WRONG8007_KEYBOARD) += -DCONFIG_WRONG8007_KEYBOARD=1
ccflags-$(CONFIG_WRONG8007_USB) += -DCONFIG_WRONG8007_USB=1
ccflags-$(CONFIG_WRONG8007_WHITELIST) += -DCONFIG_WRONG8007_WHITELIST=1
ccflags-$(CONFIG_WRONG8007_NETWORK) += -DCONFIG_WRONG8007_NETWORK=1
ccflags-$(CONFIG_WRONG8007_DEBUG) += -DDEBUG

# Optional additional compiler flags
ccflags-y += $(WB_EXTRA_CFLAGS)
