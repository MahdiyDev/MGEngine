CC = gcc
CXX = g++

CFLAGS=
CXXFLAGS=-Wall -Werror -pedantic -std=c++20

LIB_LINKS+=-lglfw3 -limgui
LIB_DIR+=-L../3rdparty/glfw/lib
3RDPARTY_GLFW_OPTIONS=

ifeq ($(OS),Windows_NT)
CURRENT_DIR += $(shell sh -c "pwd -W")
LIB_LINKS+=-lwinmm -lgdi32 -lkernel32
3RDPARTY_GLFW_OPTIONS+=-G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_MAKE_PROGRAM=make
LIB_DIR+=-L../3rdparty/imgui/lib/win32
else
CURRENT_DIR += $(shell pwd)
LIB_DIR+=-L../3rdparty/imgui/lib/linux/
endif

INCLUDES+=-I./$(SOURCE_DIR)
INCLUDES+=-I../3rdparty/glad/include
INCLUDES+=-I../3rdparty/stb
INCLUDES+=-I../3rdparty/glfw/include
INCLUDES+=-I../3rdparty/imgui/include
INCLUDES+=-I../source

SOURCE_DIR = shapes

CSOURCES = $(wildcard $(SOURCE_DIR)/*.c)
CXXSOURCES = $(wildcard $(SOURCE_DIR)/*.cpp)

COBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(SOURCE_DIR)/%,$(CSOURCES))
CXXOBJECTS = $(patsubst $(SOURCE_DIR)/%.cpp,$(SOURCE_DIR)/%,$(CXXSOURCES))
MGENGINE_OBJS = ../build/obj/glad.o \
				../build/obj/mge_core.o \
				../build/obj/mge_gl.o \
				../build/obj/mge_math.o \
				../build/obj/mge_shapes.o \
				../build/obj/mge_texture.o \
				../build/obj/mge_utils.o

all: $(COBJECTS) $(CXXOBJECTS)

$(SOURCE_DIR)/%: $(SOURCE_DIR)/%.c
	$(CXX) $(CFLAGS) $(MGENGINE_OBJS) $< -o $@ $(INCLUDES) $(LIB_DIR) $(LIB_LINKS)

$(SOURCE_DIR)/%: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(MGENGINE_OBJS) $< -o $@ $(INCLUDES) $(LIB_DIR) $(LIB_LINKS)

.PHONY: clean
clean:
	rm $(COBJECTS) $(CXXOBJECTS)