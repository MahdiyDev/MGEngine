# MGEngine -- C11 engine (no glm, no imgui). Assimp (C++) is linked for model
# loading, so the final link pulls in libstdc++.
#
#   make              build build/MGEngine (needs the vendor/*/lib archives)
#   make make_build_dir  create build/ and copy shaders/ + assets/ into it
#   make vendor       build GLFW + Assimp from source (needs cmake + ninja)
#   make vendor-glfw / make vendor-assimp   build just one
#   make vendor-clean remove built vendor artifacts (keeps the source trees)
#   make test         build+run the unit tests (no window/GL needed)
#   make clean        remove build/ (leaves vendor/ -- use vendor-clean for that)
#
# On Windows use `mingw32-make`.

CC = gcc

VENDOR = ./vendor
MLIB   = $(VENDOR)/mlib

CFLAGS  ?= -std=c11 -Wall -Wextra -g
CPPFLAGS = -DPLATFORM_DESKTOP
INCLUDES = -I./source \
           -I$(VENDOR)/glad/include \
           -I$(VENDOR)/stb \
           -I$(VENDOR)/glfw/include \
           -I$(VENDOR)/assimp/include \
           -I$(MLIB) -I$(MLIB)/vec

LIB_DIR   = -L$(VENDOR)/glfw/lib -L$(VENDOR)/assimp/lib
# assimp before its own deps (zlibstatic, stdc++); LINK with the C++ toolchain
LIB_LINKS = -lglfw3 -lassimp -lzlibstatic
LINK      = g++

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    .SHELLFLAGS := /c
    LIB_LINKS += -lopengl32 -lgdi32 -lwinmm -lkernel32
    EXE  := .exe
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    CPDIR = if exist "$(subst /,\,$1)" xcopy /E /I /Y /Q "$(subst /,\,$1)" "$(subst /,\,$2)" >nul
    RMDIR = if exist "$(subst /,\,$1)" rmdir /s /q "$(subst /,\,$1)"
else
    LIB_LINKS += -lGL -lm -lpthread -ldl -lX11
    EXE  :=
    MKDIR = mkdir -p $1
    CPDIR = test -d "$1" && { mkdir -p "$2" && cp -r "$1"/. "$2"/; } || true
    RMDIR = rm -rf $1
endif

SOURCE_DIR    = source
BUILD_DIR     = build
BUILD_OBJ_DIR = $(BUILD_DIR)/obj

# platforms/*.c is #included by mge_core.c, so it is not compiled on its own
CSOURCES = $(wildcard $(SOURCE_DIR)/*.c)
# glad lives with the other vendored deps but is compiled into the engine
GLAD_SRC = $(VENDOR)/glad/glad.c
COBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_OBJ_DIR)/%.o,$(CSOURCES)) $(BUILD_OBJ_DIR)/glad.o

EXECUTABLE = MGEngine

.PHONY: all clean vendor vendor-glfw vendor-assimp vendor-clean test make_build_dir

all: make_build_dir $(BUILD_DIR)/$(EXECUTABLE)$(EXE)

# create build/obj and stage runtime data so build/MGEngine can run from build/
make_build_dir:
	$(call MKDIR,$(BUILD_OBJ_DIR))
	$(call CPDIR,shaders,$(BUILD_DIR)/shaders)
	$(call CPDIR,assets,$(BUILD_DIR)/assets)

# link with g++ so libstdc++ / the C++ runtime come in for Assimp
$(BUILD_DIR)/$(EXECUTABLE)$(EXE): $(COBJECTS)
	$(LINK) $(CFLAGS) $(COBJECTS) -o $@ $(LIB_DIR) $(LIB_LINKS)

$(BUILD_OBJ_DIR):
	$(call MKDIR,$(BUILD_OBJ_DIR))

$(BUILD_OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_OBJ_DIR)/glad.o: $(GLAD_SRC) | $(BUILD_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@

vendor: vendor-glfw vendor-assimp

vendor-glfw:
	cd $(VENDOR)/glfw/source && \
	cmake -S . -B ../build -DCMAKE_INSTALL_PREFIX=../ -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF && \
	cmake --build ../build && cmake --install ../build

# OBJ + glTF2 + FBX importers only, no exporters/tools/tests -> small static lib
vendor-assimp:
	cd $(VENDOR)/assimp/source && \
	cmake -S . -B ../build -DCMAKE_INSTALL_PREFIX=../ \
	  -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_ZLIB=ON \
	  -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=OFF \
	  -DASSIMP_BUILD_OBJ_IMPORTER=ON -DASSIMP_BUILD_GLTF_IMPORTER=ON -DASSIMP_BUILD_FBX_IMPORTER=ON \
	  -DASSIMP_NO_EXPORT=ON -DASSIMP_BUILD_ASSIMP_TOOLS=OFF -DASSIMP_BUILD_SAMPLES=OFF \
	  -DASSIMP_BUILD_TESTS=OFF -DASSIMP_INSTALL=ON -DASSIMP_WARNINGS_AS_ERRORS=OFF && \
	cmake --build ../build --config Release && cmake --install ../build --config Release

# drop everything `make vendor` produced; the committed source trees stay put
vendor-clean:
	$(call RMDIR,$(VENDOR)/glfw/lib)
	$(call RMDIR,$(VENDOR)/glfw/build)
	$(call RMDIR,$(VENDOR)/assimp/lib)
	$(call RMDIR,$(VENDOR)/assimp/lib64)
	$(call RMDIR,$(VENDOR)/assimp/bin)
	$(call RMDIR,$(VENDOR)/assimp/include)
	$(call RMDIR,$(VENDOR)/assimp/share)
	$(call RMDIR,$(VENDOR)/assimp/build)
	$(call RMDIR,$(VENDOR)/assimp/source/build)

test:
	$(MAKE) -C test

clean:
	$(MAKE) -C test clean
	$(call RMDIR,$(BUILD_DIR))
