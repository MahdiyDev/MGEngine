CC = gcc
CXX = g++

CFLAGS=
CXXFLAGS=-Wall -Werror -pedantic -std=c++14

LIB_LINKS+=-lglfw3

ifeq ($(OS),Windows_NT)
CURRENT_DIR += $(shell sh -c "pwd -W")
LIB_LINKS+=-lwinmm -lgdi32
LIB_DIR+=-L./3rdparty/glfw/lib
else
CURRENT_DIR += $(shell pwd)
endif

INCLUDES+=-I./$(SOURCE_DIR)
INCLUDES+=-I./3rdparty/glad/include
INCLUDES+=-I./3rdparty/glm
INCLUDES+=-I./3rdparty/stb
INCLUDES+=-I./3rdparty/glfw/include

SOURCE_DIR = source
BUILD_DIR = build
BUILD_OBJ_DIR=$(BUILD_DIR)/obj

CSOURCES = $(wildcard $(SOURCE_DIR)/*.c)
CXXSOURCES = $(wildcard $(SOURCE_DIR)/*.cpp)

COBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_OBJ_DIR)/%.o,$(CSOURCES))
CXXOBJECTS = $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_OBJ_DIR)/%.o,$(CXXSOURCES))

EXECUTABLE = MGEngine

all: make_build_dir $(EXECUTABLE)

$(EXECUTABLE): $(COBJECTS) $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) $(COBJECTS) $(CXXOBJECTS) -o $(BUILD_DIR)/$@ $(LIB_DIR) $(LIB_LINKS)

$(BUILD_OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test:
	$(CXX) test.cpp source/mge_utils.cpp source/mge_core.cpp source/glad.c \
	$(CXXFLAGS) $(INCLUDES) $(LIB_DIR) $(LIB_LINKS)

make_build_dir:
	mkdir -p $(BUILD_OBJ_DIR)
	cp -r ./assests $(BUILD_DIR)
	cp -r ./shaders $(BUILD_DIR)

gen_clangd:
	@rm -rf .clangd
	@printf "CompileFlags:\n" >> .clangd
	@printf "\tAdd:\n" >> .clangd
	@printf "    - $(CXXFLAGS)\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/${SOURCE_DIR}\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/glad/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/glfw/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/glm\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/stb\n" >> .clangd

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf build/*
