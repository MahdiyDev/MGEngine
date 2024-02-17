CXX=g++

MODE=Debug

CXXFLAGS+=-I./Dependencies/glad/include -I./Dependencies/glfw/include -I./Dependencies/glm -I./include
CXXFLAGS+=-L./Dependencies/glfw/lib
CXXFLAGS+=-l:libglfw3.a -lgdi32

FILES+=./Dependencies/glad/src/glad.c
FILES+=./src/main.cpp
FILES+=./src/Shader.cpp

all: main

main: make_build_dir
	$(CXX) $(FILES) $(CXXFLAGS) -o build/main

make_build_dir:
	cp ./Dependencies/glfw/lib/glfw3.dll ./build
	cp -r ./assests ./build
	cp -r ./shaders ./build
	mkdir -p build

ifeq ($(OS),Windows_NT)
CURRENT_DIR += $(shell sh -c "pwd -W")
else
CURRENT_DIR += $(shell pwd)
endif

gen_clangd:
	@rm .clangd
	@printf "CompileFlags:\n" >> .clangd
	@printf "\tAdd:\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)\\Dependencies\\glad\\include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)\\Dependencies\\glfw\\include\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)\\Dependencies\\glm\n" >> .clangd
	@printf "    - -I$(CURRENT_DIR)\\include\n" >> .clangd

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf build/*
