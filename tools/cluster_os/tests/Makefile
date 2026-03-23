# SPDX-License-Identifier: GPL-2.0

CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -g -D_GNU_SOURCE
CFLAGS += -I..

TEST_BINS := test_config

all: $(TEST_BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test_config: test_config.o ../config.o
	$(CC) $(CFLAGS) $^ -o $@

run_tests: all
	./test_config

clean:
	rm -f $(TEST_BINS) *.o

.PHONY: all run_tests clean
