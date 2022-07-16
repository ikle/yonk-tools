#
# Colibri Build System: Subprojects
#
# Copyright (c) 2006-2022 Alexei A. Smekalkine <ikle@ikle.ru>
#
# SPDX-License-Identifier: BSD-2-Clause
#

#
# sysroot defines
#

PREFIX	?= /usr
LIBDIR	?= $(PREFIX)/lib/$(MULTIARCH)
PCDIR	?= $(LIBDIR)/pkgconfig
SYSROOT	?= $(CURDIR)/sysroot

pathlist = $(subst $(eval) ,:,$(strip $1))

PKG_CONFIG_PATH	:= $(call pathlist,$(SYSROOT)$(PCDIR) $(PKG_CONFIG_PATH))

export SYSROOT PKG_CONFIG_PATH

#
# guarantie default target
#

.PHONY: all clean install doc

all:

#
# rules to manage subprojects
#

clean:
	$(RM) -r $(SYSROOT)

define declare-child

.PHONY: $(1) clean-$(1) install-$(1) doc-$(1)

all:             $(1)
clean:     clean-$(1)
install: install-$(1)
doc:         doc-$(1)

$(1):
	+$(MAKE) -C $(1)
	+$(MAKE) -C $(1) install DESTDIR=$(SYSROOT)

clean-$(1):
	+$(MAKE) -C $(1) clean

install-$(1):
	+$(MAKE) -C $(1) install

doc-$(1):
	+$(MAKE) -C $(1) doc

endef

$(foreach F,$(CHILDS),$(eval $(call declare-child,$(F))))
