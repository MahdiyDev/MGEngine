CC = gcc
CXX = g++

CFLAGS=
CXXFLAGS=-Wall -Werror -pedantic

LIB_LINKS+=-lglfw3

ifeq ($(OS),Windows_NT)
CURRENT_DIR += $(shell sh -c "pwd -W")
LIB_LINKS+=-lwinmm -lgdi32
else
CURRENT_DIR += $(shell pwd)
endif

INCLUDES+=-I./$(SOURCE_DIR)
INCLUDES+=-I./Dependencies/glad/include
INCLUDES+=-I./Dependencies/glm
INCLUDES+=-I./Dependencies/stb
INCLUDES+=-I./Dependencies/glfw/include
LIB_DIR+=-L./Dependencies/glfw/lib

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
	$(CXXFLAGS) $(INCLUDES) $(LIB_LINKS) \
	-I./Dependencies/glfw/include \
	-L. -lwinmm -lgdi32

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
	@printf "    - -I$(CURRENT_DIR)/Dependencies/glad/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/glfw/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/glm\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/stb\n" >> .clangd

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf build/*
