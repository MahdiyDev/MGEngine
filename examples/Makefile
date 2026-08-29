# Build the shape examples. Requires the engine objects to be built first:
#   (cd .. && make)
#
#   make            build every example in shapes/
#   make clean

CC = gcc

MLIB = ../3rdparty/mlib

CFLAGS   ?= -std=c11 -Wall -Wextra -g
CPPFLAGS  = -DPLATFORM_DESKTOP
INCLUDES  = -I../source \
            -I../3rdparty/glad/include \
            -I../3rdparty/stb \
            -I../3rdparty/glfw/include \
            -I$(MLIB) -I$(MLIB)/vec

LIB_DIR   = -L../3rdparty/glfw/lib
LIB_LINKS = -lglfw3

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    .SHELLFLAGS := /c
    LIB_LINKS += -lopengl32 -lgdi32 -lwinmm -lkernel32
    EXE = .exe
    RM  = del /q /f $(subst /,\,$1) 2>nul
else
    LIB_LINKS += -lGL -lm -lpthread -ldl -lX11
    EXE =
    RM  = rm -f $1
endif

# engine objects, minus main.o (each example has its own main)
ENGINE_OBJS = $(filter-out ../build/obj/main.o,$(wildcard ../build/obj/*.o))

SRC = $(wildcard shapes/*.c)
BIN = $(patsubst shapes/%.c,shapes/%$(EXE),$(SRC))

.PHONY: all clean
all: $(BIN)

shapes/%$(EXE): shapes/%.c $(ENGINE_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) $< $(ENGINE_OBJS) -o $@ $(LIB_DIR) $(LIB_LINKS)

clean:
	-$(call RM,$(BIN))
