# Build and run the stream.h test suite and examples.
#
#   make                build and run the test suite
#   make examples       build and run the example programs
#   make clean          remove built binaries
#
# Needs any C11 compiler; stream.h uses no compiler extensions.
# Override with `make CC=clang` or `make CFLAGS="-std=c23 -O2"`.

# override make's built-in default (cc) but honour env / command-line CC
ifeq ($(origin CC),default)
CC := gcc
endif
ROOT   := ..
CFLAGS ?= -std=c11 -Wall -Wextra -O1 -g
CPPFLAGS += -I$(ROOT)

TESTS    := test_stream
EXAMPLES := stream

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

test/%$(EXE): test/%.c $(ROOT)/test.h stream.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

example/%$(EXE): example/%.c stream.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	-$(RM) $(call native,$(BINS))
