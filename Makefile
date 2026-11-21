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

# Build profile; .config when present
PROFILE ?= $(firstword $(wildcard $(DOTCONFIG)) $(ROOT)/configs/defconfig)
override PROFILE := $(abspath $(PROFILE))

# Runtime parameters; keep secrets outside modprobe.d for obvious reasons
PARAMS ?= /etc/wrong8007/wrong8007.conf
PARAMS_TEMPLATE := $(ROOT)/configs/wrong8007.conf.example

VERSION := $(shell sed -n 's/^PACKAGE_VERSION="\(.*\)"$$/\1/p' "$(ROOT)/dkms.conf")

# Installation paths for packages and dkms-install
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share
UNITDIR ?= $(PREFIX)/lib/systemd/system
SRCDIR := /usr/src/wrong8007-$(VERSION)

DKMS_SOURCES := Kbuild Kconfig Makefile dkms.conf \
	configs/defconfig configs/wrong8007.conf.example \
	tools/validate-config.sh tools/params.sh \
	$(patsubst $(ROOT)/%,%,$(wildcard $(ROOT)/*.c $(ROOT)/lib/*.c \
	$(ROOT)/trigger/*.c $(ROOT)/include/*.h))

.PHONY: all \
	menuconfig guiconfig oldconfig olddefconfig validate defconfig \
	params-init params-check \
	build install uninstall load load-installed remove reload clean \
	install-files dkms-install dkms-uninstall

all: build


#
# Interactive configuration through Kconfiglib.
#

menuconfig guiconfig oldconfig olddefconfig:
	@command -v $@ >/dev/null || { \
		echo "error: $@ needs Kconfiglib (pip install kconfiglib)"; \
		echo "editing $(DOTCONFIG) works too; start from: make defconfig"; \
		exit 1; \
	}
	cd "$(ROOT)" && KCONFIG_CONFIG="$(DOTCONFIG)" $@ "$(KCONFIG)"
	@$(MAKE) validate PROFILE="$(DOTCONFIG)"


#
# Apply the selected configuration profile.
#

defconfig:
	@test -f "$(CONFIG)" || { \
		echo "error: config profile not found: $(CONFIG)"; \
		exit 1; \
	}
	@cp "$(CONFIG)" "$(DOTCONFIG)"
	@sh "$(ROOT)/tools/validate-config.sh" --summary "$(DOTCONFIG)"


#
# Validate the build profile.
#

validate:
	@sh "$(ROOT)/tools/validate-config.sh" "$(PROFILE)"


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
# Validate parameters against the build profile.
#

params-check:
	@sudo sh "$(ROOT)/tools/params.sh" check "$(PROFILE)" "$(PARAMS)"


#
# Build only when enabled in the build profile.
#

build: validate
	@if grep -q '^CONFIG_WRONG8007=y' "$(PROFILE)"; then \
		$(MAKE) -C "$(KDIR)" M="$(ROOT)" WRONG8007_PROFILE="$(PROFILE)" modules; \
	else \
		$(MAKE) -C "$(KDIR)" M="$(ROOT)" clean; \
		echo "module disabled; no module built."; \
	fi


#
# Install the module into the running kernel's module tree.
#

install: build
	sudo $(MAKE) -C "$(KDIR)" M="$(ROOT)" WRONG8007_PROFILE="$(PROFILE)" modules_install
	sudo depmod -a


#
# Load the locally built module with configured parameters.
#
# Warning: insmod parameters may be visible in process listings.
#

load: build
	@test -f "$(ROOT)/wrong8007.ko" || { \
		echo "error: no $(ROOT)/wrong8007.ko built."; \
		echo "the module is disabled in the build profile."; \
		exit 1; \
	}
	@sudo sh "$(ROOT)/tools/params.sh" insmod "$(PROFILE)" "$(PARAMS)" "$(ROOT)/wrong8007.ko"


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
# Install the DKMS source, parameter check and systemd unit.
#
# Packages run it with DESTDIR and PREFIX=/usr.
#

install-files:
	@test -n "$(VERSION)" || { echo "error: no PACKAGE_VERSION in dkms.conf"; exit 1; }
	@cd "$(ROOT)" && for f in $(DKMS_SOURCES); do \
		install -Dm644 "$$f" "$(DESTDIR)$(SRCDIR)/$$f" || exit 1; \
	done
	install -Dm755 "$(ROOT)/tools/params.sh" "$(DESTDIR)$(BINDIR)/wrong8007-params"
	install -Dm644 "$(ROOT)/packaging/wrong8007.service" "$(DESTDIR)$(UNITDIR)/wrong8007.service"
	install -Dm644 "$(PARAMS_TEMPLATE)" "$(DESTDIR)$(DATADIR)/wrong8007/wrong8007.conf.example"
	install -d -m 0700 "$(DESTDIR)/etc/wrong8007"


#
# Build and install the module for the running kernel through DKMS.
#
# DKMS rebuilds it for every kernel installed afterwards.
#

dkms-install:
	@test -n "$(VERSION)" || { echo "error: no PACKAGE_VERSION in dkms.conf"; exit 1; }
	@command -v dkms >/dev/null || { \
		echo "error: dkms not found; install it from your distribution"; \
		exit 1; \
	}
	-@sudo dkms remove "wrong8007/$(VERSION)" --all >/dev/null 2>&1
	sudo rm -rf "$(SRCDIR)"
	sudo $(MAKE) -C "$(ROOT)" BINDIR="$(BINDIR)" DATADIR="$(DATADIR)" UNITDIR="$(UNITDIR)" install-files
	sudo dkms install "wrong8007/$(VERSION)"
	-@sudo systemctl daemon-reload 2>/dev/null


#
# Remove the DKMS module and the installed files.
#
# The parameter file in /etc/wrong8007 stays.
#

dkms-uninstall: remove
	@test -n "$(VERSION)" || { echo "error: no PACKAGE_VERSION in dkms.conf"; exit 1; }
	-@sudo systemctl disable --now wrong8007.service >/dev/null 2>&1
	-sudo dkms remove "wrong8007/$(VERSION)" --all
	sudo rm -rf "$(SRCDIR)" "$(DATADIR)/wrong8007"
	sudo rm -f "$(BINDIR)/wrong8007-params" "$(UNITDIR)/wrong8007.service"
	-@sudo systemctl daemon-reload 2>/dev/null


#
# Remove build artifacts.
#

clean:
	$(MAKE) -C "$(KDIR)" M="$(ROOT)" WRONG8007_PROFILE="$(PROFILE)" clean
