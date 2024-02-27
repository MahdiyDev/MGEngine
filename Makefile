CC = gcc
CXX = g++

CXXFLAGS=-Wall -Werror

INCLUDES=-I./Dependencies/glad/include -I./Dependencies/glm -I./Dependencies/stb
LIB_LINKS=-lglfw3

ifeq ($(OS),Windows_NT)
CURRENT_DIR += $(shell sh -c "pwd -W")
LIB_LINKS+=-lgdi32
else
CURRENT_DIR += $(shell pwd)
endif

SRCDIR = src
BUILDDIR = build
BUILD_OBJ_DIR=$(BUILDDIR)/obj

CSOURCES := $(wildcard $(SRCDIR)/*.c)
CXXSOURCES := $(wildcard $(SRCDIR)/*.cpp)

COBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILD_OBJ_DIR)/%.o,$(CSOURCES))
CXXOBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(BUILD_OBJ_DIR)/%.o,$(CXXSOURCES))

EXECUTABLE = MGEngine

all: make_build_dir $(EXECUTABLE)

$(EXECUTABLE): $(COBJECTS) $(CXXOBJECTS)
	$(CXX) $(CXXFLAGS) $(COBJECTS) $(CXXOBJECTS) -o $(BUILDDIR)/$@ $(LIB_LINKS)

$(BUILD_OBJ_DIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_OBJ_DIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

make_build_dir:
	mkdir -p $(BUILD_OBJ_DIR)
	cp -r ./assests $(BUILDDIR)
	cp -r ./shaders $(BUILDDIR)

gen_clangd:
	@rm -rf .clangd
	@printf "CompileFlags:\n" >> .clangd
	@printf "\tAdd:\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/glad/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/glfw/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/glm\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/Dependencies/stb\n" >> .clangd

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf build/*
