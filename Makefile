# SPDX-License-Identifier: GPL-2.0
#
# wrong8007
#
# Copyright (c) 2023, 03C0 (https://03c0.net/)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses/>.

KDIR ?= /lib/modules/$(shell uname -r)/build

ROOT := $(CURDIR)

KCONFIG := $(ROOT)/Kconfig
DOTCONFIG := $(ROOT)/.config

# Default profile used to seed .config
CONFIG ?= $(ROOT)/configs/defconfig

# Runtime parameters; keep secrets outside modprobe.d for obvious reasons
PARAMS ?= /etc/wrong8007/wrong8007.conf
PARAMS_TEMPLATE := $(ROOT)/configs/wrong8007.conf.example

CONFIG_DIR := $(ROOT)/include/config
GENERATED_DIR := $(ROOT)/include/generated

AUTOCONFIG := $(CONFIG_DIR)/auto.conf
AUTOHEADER := $(GENERATED_DIR)/wrong8007_config.h

# Use the kernel tree's Kconfig host binary
CONF_BIN := $(KDIR)/scripts/kconfig/conf

# Kconfig input and output paths
KCONFIG_ENV := KBUILD_KCONFIG="$(KCONFIG)" \
	KCONFIG_CONFIG="$(DOTCONFIG)" \
	KCONFIG_AUTOCONFIG="$(AUTOCONFIG)" \
	KCONFIG_AUTOHEADER="$(AUTOHEADER)"

.PHONY: all \
	nconfig menuconfig oldconfig config validate defconfig \
	params-init params-check \
	build install uninstall load load-installed remove reload clean

all: build


#
# Interactive Kconfig configuration.
#

nconfig:
	$(KCONFIG_ENV) $(MAKE) -C "$(KDIR)" nconfig
	@$(MAKE) config
	@$(MAKE) validate

menuconfig:
	$(KCONFIG_ENV) $(MAKE) -C "$(KDIR)" menuconfig
	@$(MAKE) config
	@$(MAKE) validate

oldconfig:
	$(KCONFIG_ENV) $(MAKE) -C "$(KDIR)" oldconfig
	@$(MAKE) config
	@$(MAKE) validate


#
# Generate Kconfig outputs from .config using the kernel's conf binary.
#

config:
	@test -f "$(DOTCONFIG)" || { \
		echo "no $(DOTCONFIG); using $(CONFIG)."; \
		cp "$(CONFIG)" "$(DOTCONFIG)"; \
	}
	@test -x "$(CONF_BIN)" || { \
		echo "error: kernel Kconfig host binary not found: $(CONF_BIN)"; \
		echo "build it as root: sudo make -C \"$(KDIR)\" scripts/kconfig/conf"; \
		exit 1; \
	}
	@mkdir -p "$(CONFIG_DIR)" "$(GENERATED_DIR)"
	cd "$(ROOT)" && \
	KCONFIG_CONFIG="$(DOTCONFIG)" \
	KCONFIG_AUTOCONFIG="$(AUTOCONFIG)" \
	KCONFIG_AUTOHEADER="$(AUTOHEADER)" \
		"$(CONF_BIN)" --syncconfig "$(KCONFIG)"


#
# Apply the selected configuration profile.
#

defconfig:
	@test -f "$(CONFIG)" || { \
		echo "error: config profile not found: $(CONFIG)"; \
		exit 1; \
	}
	@cp "$(CONFIG)" "$(DOTCONFIG)"
	@$(MAKE) config
	@sh "$(ROOT)/tools/validate-config.sh" --summary "$(DOTCONFIG)"


#
# Validate the build profile.
#

validate:
	@sh "$(ROOT)/tools/validate-config.sh" "$(DOTCONFIG)"


#
# Install the parameter template as a root-only file if absent.
#

params-init:
	@if sudo test -e "$(PARAMS)"; then \
		echo "$(PARAMS) already exists; edit it with: sudoedit $(PARAMS)"; \
	else \
		sudo install -d -m 0700 -o root -g root "$(dir $(PARAMS))" && \
		sudo install -m 0600 -o root -g root "$(PARAMS_TEMPLATE)" "$(PARAMS)" && \
		echo "installed $(PARAMS) (mode 0600); edit it with: sudoedit $(PARAMS)"; \
	fi


#
# Validate parameters against the selected Kconfig profile.
#

params-check: config
	@sudo sh "$(ROOT)/tools/params.sh" check "$(DOTCONFIG)" "$(PARAMS)"


#
# Build only when enabled in .config.
#

build: config validate
	@if grep -q '^CONFIG_WRONG8007=y' "$(DOTCONFIG)"; then \
		$(MAKE) -C "$(KDIR)" M="$(ROOT)" modules; \
	else \
		$(MAKE) -C "$(KDIR)" M="$(ROOT)" clean; \
		echo "module disabled; no module built."; \
	fi


#
# Install the module into the running kernel's module tree.
#

install: build
	sudo $(MAKE) -C "$(KDIR)" M="$(ROOT)" modules_install
	sudo depmod -a


#
# Load the locally built module with configured parameters.
#
# Warning: insmod parameters may be visible in process listings.
#

load: build
	@test -f "$(ROOT)/wrong8007.ko" || { \
		echo "error: no $(ROOT)/wrong8007.ko built."; \
		echo "the module is disabled in the current Kconfig profile."; \
		exit 1; \
	}
	@sudo sh "$(ROOT)/tools/params.sh" insmod "$(DOTCONFIG)" "$(PARAMS)" "$(ROOT)/wrong8007.ko"


#
# Load the installed module using the dedicated configuration.
#

load-installed: params-check
	sudo modprobe -C "$(PARAMS)" wrong8007


#
# Remove the module if it is currently loaded.
#

remove:
	@if sudo lsmod 2>/dev/null | grep -q '^wrong8007 '; then \
		sudo rmmod wrong8007; \
	else \
		echo "wrong8007 is not loaded."; \
	fi


#
# Remove installed module files and refresh module dependencies.
#

uninstall: remove
	-sudo find "/lib/modules/$(shell uname -r)" -name 'wrong8007.ko*' -delete
	sudo depmod -a


#
# Rebuild and reload the local module.
#

reload:
	@$(MAKE) remove
	@$(MAKE) load


#
# Remove build artifacts and generated Kconfig files.
#

clean:
	$(MAKE) -C "$(KDIR)" M="$(ROOT)" clean
	rm -rf "$(CONFIG_DIR)" "$(GENERATED_DIR)"
