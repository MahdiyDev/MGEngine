CC = gcc
CXX = g++

CFLAGS=
CXXFLAGS=-Wall -Werror -pedantic -std=c++14

LIB_LINKS+=-lglfw3 -limgui
LIB_DIR+=-L./3rdparty/glfw/lib

ifeq ($(OS),Windows_NT)
CURRENT_DIR += $(shell sh -c "pwd -W")
LIB_LINKS+=-lwinmm -lgdi32 -lkernel32
LIB_DIR+=-L./3rdparty/imgui/lib/win32
else
CURRENT_DIR += $(shell pwd)
LIB_DIR+=-L./3rdparty/imgui/lib/linux/
endif

INCLUDES+=-I./$(SOURCE_DIR)
INCLUDES+=-I./3rdparty/glad/include
INCLUDES+=-I./3rdparty/stb
INCLUDES+=-I./3rdparty/glfw/include
INCLUDES+=-I./3rdparty/imgui/include

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

.PHONY: 3rdparty
3rdparty:
	cd 3rdparty/glfw/source && \
	cmake -S . -B ../build -DCMAKE_INSTALL_PREFIX=../ && \
	cd ../build && \
	make && make install 

test: make_build_dir $(COBJECTS) $(CXXOBJECTS)
	$(CXX) $(INCLUDES) -c test.cpp -o $(BUILD_OBJ_DIR)/test.o
	$(CXX) $(CXXFLAGS) $(COBJECTS) $(filter-out $(BUILD_OBJ_DIR)/main.o,$(CXXOBJECTS)) $(BUILD_OBJ_DIR)/test.o $(LIB_DIR) $(LIB_LINKS)

make_build_dir:
	mkdir -p $(BUILD_OBJ_DIR)
	cp -r ./assets $(BUILD_DIR)
	cp -r ./shaders $(BUILD_DIR)

gen_clangd:
	@rm -rf .clangd
	@printf "CompileFlags:\n" >> .clangd
	@printf "\tAdd:\n" >> .clangd
	@printf "    - $(CXXFLAGS)\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/${SOURCE_DIR}\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/glad/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/glfw/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/imgui/include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)/3rdparty/stb\n" >> .clangd

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf build/*
	rm -rf 3rdparty/glfw/build
