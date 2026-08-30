# Build and run the async test suites and examples.
#
#   make                build and run the test suites
#   make examples       build and run the example programs
#   make clean          remove built binaries
#
# Needs any C11 compiler; async.h uses no compiler extensions.
# async_fs.h also needs string.h / vec.h, hence -I../vec below.

# override make's built-in default (cc) but honour env / command-line CC
ifeq ($(origin CC),default)
CC := gcc
endif
ROOT   := ..
CFLAGS ?= -std=c11 -Wall -Wextra -O1 -g
CPPFLAGS += -I$(ROOT) -I$(ROOT)/vec

TESTS    := test_async test_async_fs
EXAMPLES := async async_fs

ifeq ($(OS),Windows_NT)
    EXE    := .exe
    RM     := del /q /f
    native  = $(subst /,\,$1)
else
    EXE    :=
    RM     := rm -f
    native  = $1
endif

BINS := $(addsuffix $(EXE),$(addprefix test/,$(TESTS)) $(addprefix example/,$(EXAMPLES)))

.PHONY: all test examples clean $(TESTS) $(EXAMPLES)

all: test

test: $(TESTS)

examples: $(EXAMPLES)

$(TESTS): %: test/%$(EXE)
	$(call native,$<)

$(EXAMPLES): %: example/%$(EXE)
	$(call native,$<)

test/%$(EXE): test/%.c $(ROOT)/test.h async.h async_fs.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

example/%$(EXE): example/%.c async.h async_fs.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	-$(RM) $(call native,$(BINS))
	-$(RM) $(call native,example/*.tmp test/*.tmp)
