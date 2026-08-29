# Unit tests for the parts of MGEngine that need no window / GL context.
#
#   make            build and run every suite
#   make clean

ifeq ($(origin CC),default)
CC := gcc
endif

MLIB   := ../3rdparty/mlib
CFLAGS ?= -std=c11 -Wall -Wextra -O1 -g
CPPFLAGS += -DPLATFORM_DESKTOP -I../source -I$(MLIB) -I$(MLIB)/vec

TESTS := test_math test_utils test_object

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    .SHELLFLAGS := /c
    EXE    := .exe
    RM     := del /q /f 2>nul
    native  = $(subst /,\,$1)
    runexe  = .\$(subst /,\,$1)
else
    EXE    :=
    RM     := rm -f
    native  = $1
    runexe  = ./$1
endif

.PHONY: all test clean $(TESTS)

all: test
test: $(TESTS)

$(TESTS): %: %$(EXE)
	$(call runexe,$<)

test_math$(EXE): test_math.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ test_math.c ../source/mge_math.c -lm

test_utils$(EXE): test_utils.c ../source/mge_utils.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ test_utils.c ../source/mge_utils.c

test_object$(EXE): test_object.c ../source/mge_object.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ test_object.c ../source/mge_object.c ../source/mge_math.c -lm

clean:
	-$(RM) $(call native,$(addsuffix $(EXE),$(TESTS)) _mge_utils.tmp)
