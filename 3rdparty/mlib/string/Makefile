# Build and run the string.h test suite and examples.
#
#   make                build and run the test suite
#   make examples       build and run the example programs
#   make clean          remove built binaries
#
# Needs any C11 compiler; string.h / vec.h use no compiler extensions.
# Override with `make CC=clang` or `make CFLAGS="-std=c23 -O2"`.

# override make's built-in default (cc) but honour env / command-line CC
ifeq ($(origin CC),default)
CC := gcc
endif
ROOT   := ..
VEC    := ../vec
CFLAGS ?= -std=c11 -Wall -Wextra -O1 -g
CPPFLAGS += -I$(ROOT) -I$(VEC)

TESTS    := test_string
EXAMPLES := string_builder string_view

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

test/%$(EXE): test/%.c $(ROOT)/test.h $(VEC)/vec.h string.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

example/%$(EXE): example/%.c $(VEC)/vec.h string.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	-$(RM) $(call native,$(BINS))
