-include $(src)/include/config/auto.conf

# External module builds link obj-m
obj-m += $(if $(filter y m,$(CONFIG_WRONG8007)),wrong8007.o)

wrong8007-objs := core.o lib/phrase_match.o lib/parse.o

wrong8007-$(CONFIG_WRONG8007_KEYBOARD) += trigger/keyboard.o
wrong8007-$(CONFIG_WRONG8007_USB) += trigger/usb.o
wrong8007-$(CONFIG_WRONG8007_NETWORK) += trigger/network.o

ccflags-y += -I$(src)/include -I$(src)/include/generated

ccflags-$(CONFIG_WRONG8007_DEBUG) += -DDEBUG

# Optional additional compiler flags
ccflags-y += $(WB_EXTRA_CFLAGS)
