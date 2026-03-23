# SPDX-License-Identifier: GPL-2.0

include ../scripts/Makefile.include

sbindir ?= /usr/sbin

ifeq ($(srctree),)
srctree := $(patsubst %/,%,$(dir $(CURDIR)))
srctree := $(patsubst %/,%,$(dir $(srctree)))
endif

MAKEFLAGS += -r

override CFLAGS += -O2 -Wall -Wextra -g -D_GNU_SOURCE

ALL_TARGETS := cluster_osd cluster_os_admin
ALL_PROGRAMS := $(patsubst %,$(OUTPUT)%,$(ALL_TARGETS))

all: $(ALL_PROGRAMS)

$(OUTPUT)%.o: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT)cluster_osd: $(OUTPUT)cluster_osd.o $(OUTPUT)config.o
	$(QUIET_LINK)$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OUTPUT)cluster_os_admin: $(OUTPUT)admin_app.o $(OUTPUT)config.o
	$(QUIET_LINK)$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test: all
	$(Q)$(MAKE) --no-print-directory -C tests run_tests

clean:
	rm -f $(ALL_PROGRAMS)
	find $(or $(OUTPUT),.) -name '*.o' -delete -o -name '\.*.d' -delete -o -name '\.*.cmd' -delete
	$(Q)$(MAKE) --no-print-directory -C tests clean

install: $(ALL_PROGRAMS)
	install -d -m 755 $(DESTDIR)$(sbindir)
	install $(ALL_PROGRAMS) -m 755 $(DESTDIR)$(sbindir)

.PHONY: all clean install test
