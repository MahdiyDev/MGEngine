# MGEngine build.
#
# The engine is a shared library; `editor/` is a separate app (the scene editor)
# that links against it through the public headers in `source/`.
#
#   make              -> build/{libmgengine.(dll|so),editor,mgeplayer}
#                        DEBUG build: -O0 -g, assertions on
#   make release      -> build/release/{...}  PRODUCTION build: -O2 -DNDEBUG,
#                        stripped, dead code removed. Separate object cache, so
#                        `make` and `make release` coexist -- run both to
#                        keep a debug and a release set side by side.
#   make lib          -> just the engine library (of the current config)
#   make vendor       build GLFW + Assimp from source (needs cmake + ninja)
#   make vendor-glfw / make vendor-assimp   build just one
#   make vendor-clean remove built vendor artifacts (keeps the source trees)
#   make test         build+run the unit tests (no window/GL needed)
#   make clean-obj    drop the object cache of the current config
#   make clean        remove build/ (leaves vendor/ -- use vendor-clean for that)
#
# On Windows use `mingw32-make`.

CC  = gcc
CXX = g++

VENDOR = ./vendor
MLIB   = $(VENDOR)/mlib

CFLAGS   ?= -std=c11 -Wall -Wextra -g
CXXFLAGS ?= -std=c++17 -Wall -Wextra -g
CPPFLAGS  = -DPLATFORM_DESKTOP

# production build flags (see the `release` target). -O2 is the big FPS win over
# the default -O0; NDEBUG drops assertions; the section flags + -s let the linker
# garbage-collect unused code and strip symbols.
RELEASE_CFLAGS   = -std=c11   -Wall -Wextra -O2 -DNDEBUG -ffunction-sections -fdata-sections
RELEASE_CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -DNDEBUG -ffunction-sections -fdata-sections
INCLUDES  = -I./source \
            -I$(VENDOR)/glad/include \
            -I$(VENDOR)/stb \
            -I$(VENDOR)/glfw/include \
            -I$(VENDOR)/assimp/include \
            -I$(VENDOR)/imgui \
            -I$(MLIB) -I$(MLIB)/vec

LIB_DIR   = -L$(VENDOR)/glfw/lib -L$(VENDOR)/assimp/lib
LINK      = g++

SOURCE_DIR    = source
BUILD_DIR     = build
# debug lands in build/ ; `make release` (RELEASE=1) lands in build/release/ --
# separate object caches, so the two configs coexist without a wipe.
ifdef RELEASE
    CONF_DIR := $(BUILD_DIR)/release
else
    CONF_DIR := $(BUILD_DIR)
endif
BUILD_OBJ_DIR = $(CONF_DIR)/obj

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    .SHELLFLAGS := /c
    LIB_LINKS := -lglfw3 -lassimp -lzlibstatic -lopengl32 -lgdi32 -lwinmm -lkernel32 -lcomdlg32 -lshell32 -lole32
    EXE  := .exe
    PICFLAG :=
    LIB_NAME := libmgengine.dll
    IMPLIB   := $(CONF_DIR)/libmgengine.dll.a
    # bake the C/C++/pthread runtimes into the DLL so the app needs only
    # libmgengine.dll + system DLLs at run time
    SHAREDFLAGS := -shared -static -static-libgcc -static-libstdc++ -Wl,--out-implib,$(IMPLIB)
    APP_LIBS := -L$(CONF_DIR) -lmgengine
    MKDIR = if not exist "$(subst /,\,$1)" mkdir "$(subst /,\,$1)"
    CPDIR = if exist "$(subst /,\,$1)" xcopy /E /I /Y /Q "$(subst /,\,$1)" "$(subst /,\,$2)" >nul
    CPHDR = copy /Y "$(subst /,\,$1)\*.h" "$(subst /,\,$2)\" >nul
    RMDIR = if exist "$(subst /,\,$1)" rmdir /s /q "$(subst /,\,$1)"
else
    LIB_LINKS := -lglfw3 -lassimp -lzlibstatic -lGL -lm -lpthread -ldl -lX11
    EXE  :=
    PICFLAG := -fPIC
    LIB_NAME := libmgengine.so
    IMPLIB   :=
    SHAREDFLAGS := -shared
    APP_LIBS := -L$(CONF_DIR) -lmgengine -Wl,-rpath,'$$ORIGIN'
    MKDIR = mkdir -p $1
    CPDIR = test -d "$1" && { mkdir -p "$2" && cp -r "$1"/. "$2"/; } || true
    CPHDR = cp $1/*.h "$2"/
    RMDIR = rm -rf $1
endif

# set by the `release` target (via a recursive $(MAKE) ... RELEASE=1). Picks the
# production flags here so the object cache under build/release/obj/ is built
# with them -- no shared cache, no wipe.
ifdef RELEASE
    CFLAGS      := $(RELEASE_CFLAGS)
    CXXFLAGS    := $(RELEASE_CXXFLAGS)
    SHAREDFLAGS += -s -Wl,--gc-sections
    APP_EXTRA   := -s -Wl,--gc-sections
else
    APP_EXTRA   :=
endif

# every source/*.{c,cpp} is engine code (the editor app lives in editor/); the
# desktop platform file is #included by mge_core.c, not compiled on its own.
# mge_gui.cpp is the one C++ unit (Dear ImGui backend).
CSOURCES   = $(wildcard $(SOURCE_DIR)/*.c)
CXXSOURCES = $(wildcard $(SOURCE_DIR)/*.cpp)
# glad + Dear ImGui: vendored source compiled straight into the engine
GLAD_SRC  = $(VENDOR)/glad/glad.c
IMGUI_SRC = $(wildcard $(VENDOR)/imgui/*.cpp)
IMGUI_OBJ = $(patsubst $(VENDOR)/imgui/%.cpp,$(BUILD_OBJ_DIR)/imgui/%.o,$(IMGUI_SRC))
COBJECTS = $(patsubst $(SOURCE_DIR)/%.c,$(BUILD_OBJ_DIR)/%.o,$(CSOURCES)) \
           $(patsubst $(SOURCE_DIR)/%.cpp,$(BUILD_OBJ_DIR)/%.o,$(CXXSOURCES)) \
           $(IMGUI_OBJ) $(BUILD_OBJ_DIR)/glad.o

ENGINE_LIB = $(CONF_DIR)/$(LIB_NAME)
APP        = $(CONF_DIR)/editor$(EXE)
PLAYER     = $(CONF_DIR)/mgeplayer$(EXE)

.PHONY: all lib release clean clean-obj vendor vendor-glfw vendor-assimp vendor-clean test make_build_dir

all: make_build_dir $(APP) $(PLAYER)
lib: make_build_dir $(ENGINE_LIB)

# production build: -O2 -DNDEBUG, stripped, dead code removed. Its own object
# cache + outputs under build/release/, so `make` (debug) and `make release`
# coexist -- run both to have both.
release:
	$(MAKE) all RELEASE=1

# create <conf>/obj, stage runtime data + the public headers next to the app so
# it runs -- and so an editor project can point its compile_flags.txt here for
# scene-script IntelliSense -- straight from <conf>/
make_build_dir:
	$(call MKDIR,$(BUILD_OBJ_DIR))
	$(call MKDIR,$(CONF_DIR)/include)
	$(call CPHDR,$(SOURCE_DIR),$(CONF_DIR)/include)
	$(call CPDIR,shaders,$(CONF_DIR)/shaders)
	$(call CPDIR,assets,$(CONF_DIR)/assets)

# --- engine library ---
$(ENGINE_LIB): $(COBJECTS)
	$(LINK) $(CFLAGS) $(SHAREDFLAGS) -o $@ $(COBJECTS) $(LIB_DIR) $(LIB_LINKS)

$(BUILD_OBJ_DIR):
	$(call MKDIR,$(BUILD_OBJ_DIR))

# -MMD -MP writes a .d beside each .o listing the headers it #included, so editing
# a header (e.g. mge.h, which changes struct sizes) rebuilds every dependent .o.
# Without this a stale object cache silently mixes ABIs -> memory corruption.
DEPFLAGS = -MMD -MP

$(BUILD_OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $(PICFLAG) $(INCLUDES) -c $< -o $@

$(BUILD_OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp | $(BUILD_OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) $(PICFLAG) $(INCLUDES) -c $< -o $@

$(BUILD_OBJ_DIR)/glad.o: $(GLAD_SRC) | $(BUILD_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) $(PICFLAG) $(INCLUDES) -c $< -o $@

-include $(COBJECTS:.o=.d)

# vendored Dear ImGui -- warnings silenced, own object subdir
$(BUILD_OBJ_DIR)/imgui:
	$(call MKDIR,$(BUILD_OBJ_DIR)/imgui)

$(BUILD_OBJ_DIR)/imgui/%.o: $(VENDOR)/imgui/%.cpp | $(BUILD_OBJ_DIR)/imgui
	$(CXX) $(CPPFLAGS) -std=c++17 -O2 -w -ffunction-sections -fdata-sections $(DEPFLAGS) $(PICFLAG) $(INCLUDES) -c $< -o $@

# --- editor app: a plain-C consumer of the library + its headers ---
EDITOR_SRC = $(wildcard editor/*.c)
$(APP): $(EDITOR_SRC) $(wildcard editor/*.h) $(wildcard $(SOURCE_DIR)/*.h) $(ENGINE_LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(SOURCE_DIR) $(EDITOR_SRC) -o $@ $(APP_LIBS) $(APP_EXTRA)

# --- standalone player: runs a built project. Reuses the editor's data layer
#     (no GUI); what `Build Release` ships as <name>.exe.
PLAYER_SRC = runtime/player.c editor/scene.c editor/scene_io.c editor/project.c \
             editor/project_io.c editor/pathutil.c editor/editor_camera.c editor/scene_runtime.c
$(PLAYER): $(PLAYER_SRC) $(wildcard editor/*.h) $(wildcard $(SOURCE_DIR)/*.h) $(ENGINE_LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(SOURCE_DIR) -Ieditor -I$(MLIB) $(PLAYER_SRC) -o $@ $(APP_LIBS) $(APP_EXTRA)

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

clean-obj:
	$(call RMDIR,$(BUILD_OBJ_DIR))

clean:
	$(MAKE) -C test clean
	$(call RMDIR,$(BUILD_DIR))
