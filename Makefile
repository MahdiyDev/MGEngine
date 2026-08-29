# MGEngine -- pure C11 build (no C++, no glm, no imgui).
#
#   make              build build/MGEngine (needs 3rdparty/glfw/lib/libglfw3.a)
#   make 3rdparty     build GLFW from source (needs cmake)
#   make test         build+run the unit tests (no window/GL needed)
#   make clean
#
# On Windows use `mingw32-make`.

CC = gcc

MLIB = ./3rdparty/mlib

CFLAGS  ?= -std=c11 -Wall -Wextra -g
CPPFLAGS = -DPLATFORM_DESKTOP
INCLUDES = -I./source \
           -I./3rdparty/glad/include \
           -I./3rdparty/stb \
           -I./3rdparty/glfw/include \
           -I$(MLIB) -I$(MLIB)/vec

LIB_DIR   = -L./3rdparty/glfw/lib
LIB_LINKS = -lglfw3

ifeq ($(OS),Windows_NT)
    LIB_LINKS += -lopengl32 -lgdi32 -lwinmm -lkernel32
    EXE  := .exe
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
else
    LIB_LINKS += -lGL -lm -lpthread -ldl -lX11
    EXE  :=
    MKDIR = mkdir -p $1
endif

SOURCE_DIR    = source
BUILD_DIR     = build
BUILD_OBJ_DIR = $(BUILD_DIR)/obj

# platforms/*.c is #included by mge_core.c, so it is not compiled on its own
CSOURCES = $(wildcard $(SOURCE_DIR)/*.c)
COBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_OBJ_DIR)/%.o,$(CSOURCES))

EXECUTABLE = MGEngine

.PHONY: all clean 3rdparty test

# Run the built executable from the repo root so shaders/ and assets/ resolve.
all: $(BUILD_DIR)/$(EXECUTABLE)$(EXE)

$(BUILD_DIR)/$(EXECUTABLE)$(EXE): $(COBJECTS)
	$(CC) $(CFLAGS) $(COBJECTS) -o $@ $(LIB_DIR) $(LIB_LINKS)

$(BUILD_OBJ_DIR):
	$(call MKDIR,$(BUILD_OBJ_DIR))

$(BUILD_OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INCLUDES) -c $< -o $@

3rdparty:
	cd 3rdparty/glfw/source && \
	cmake -S . -B ../build -DCMAKE_INSTALL_PREFIX=../ -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF && \
	cmake --build ../build && cmake --install ../build

test:
	$(MAKE) -C test

clean:
	$(MAKE) -C test clean
ifeq ($(OS),Windows_NT)
	if exist "$(subst /,\,$(BUILD_DIR))" rmdir /s /q "$(subst /,\,$(BUILD_DIR))"
else
	rm -rf $(BUILD_DIR)
endif
