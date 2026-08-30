# Build the examples. Requires the engine objects to be built first:
#   (cd .. && make)
#
#   make            build every example in shapes/, objects/ and lighting/
#   make clean

CC   = gcc
LINK = g++   # engine links Assimp (C++), so drive the link with g++

MLIB = ../vendor/mlib

CFLAGS   ?= -std=c11 -Wall -Wextra -g
CPPFLAGS  = -DPLATFORM_DESKTOP
INCLUDES  = -I../source \
            -I../vendor/glad/include \
            -I../vendor/stb \
            -I../vendor/glfw/include \
            -I../vendor/assimp/include \
            -I$(MLIB) -I$(MLIB)/vec

LIB_DIR   = -L../vendor/glfw/lib -L../vendor/assimp/lib
LIB_LINKS = -lglfw3 -lassimp -lzlibstatic

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

# the engine objects staged by the top-level `make` (each example has its own
# main; the GUI/ImGui unit is only for the builder app)
ENGINE_OBJS = $(filter-out ../build/obj/mge_gui.o,$(wildcard ../build/obj/*.o))

SRC = $(wildcard shapes/*.c objects/*.c lighting/*.c materials/*.c meshes/*.c models/*.c depth/*.c stencil/*.c culling/*.c framebuffer/*.c)
BIN = $(SRC:.c=$(EXE))

.PHONY: all clean
all: $(BIN)

# compile the example as C11 with gcc, then link with g++ (Assimp is C++)
%$(EXE): %.c $(ENGINE_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $(<:.c=.o)
	$(LINK) $(CFLAGS) $(<:.c=.o) $(ENGINE_OBJS) -o $@ $(LIB_DIR) $(LIB_LINKS)

clean:
	-$(call RM,$(BIN) $(SRC:.c=.o))
