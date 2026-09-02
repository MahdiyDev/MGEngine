# Unit tests for the parts of MGEngine that need no window / GL context.
#
#   make            build and run every suite
#   make model      build and run test_model (needs `make vendor` for Assimp)
#   make render     headless screenshot smoke test (needs the root `make` +
#                   `make vendor`, a GPU and a desktop session)
#   make clean

ifeq ($(origin CC),default)
CC := gcc
endif
CXX ?= g++

MLIB   := ../vendor/mlib
ASSIMP := ../vendor/assimp
CFLAGS ?= -std=c11 -Wall -Wextra -O1 -g
# strict -std=c11 hides POSIX APIs (S_IFDIR, readlink, strdup, ...) on glibc
CPPFLAGS += -DPLATFORM_DESKTOP -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -I../source -I$(MLIB) -I$(MLIB)/vec

TESTS := test_math test_utils test_object test_material test_light test_mesh test_depth test_stencil test_cull test_framebuffer test_cubemap test_geometry test_instancing test_msaa test_gamma test_shadow test_gl test_debug test_gizmo test_scene_io test_project_io test_dylib test_pak test_physics test_component

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    .SHELLFLAGS := /c
    EXE    := .exe
    RM     := del /q /f 2>nul
    native  = $(subst /,\,$1)
    runexe  = .\$(subst /,\,$1)
    # link the runtime in: some Windows app-control (Device Guard / WDAC)
    # policies block freshly built, dynamically linked exes
    LDFLAGS ?= -static
    RENDER_SYSLIBS := -lopengl32 -lgdi32 -lwinmm -lkernel32 -lcomdlg32 -lshell32 -lole32
else
    EXE    :=
    RM     := rm -f
    native  = $1
    runexe  = ./$1
    LDFLAGS ?=
    RENDER_SYSLIBS := -lGL -lm -lpthread -ldl -lX11
endif

.PHONY: all test model render clean $(TESTS)

all: test
test: $(TESTS)

$(TESTS): %: %$(EXE)
	$(call runexe,$<)

# not part of `make test`: needs the vendored Assimp lib (run `make vendor`)
model: test_model$(EXE)
	$(call runexe,test_model$(EXE))

# not part of `make test`: opens a real (hidden) GL window and renders a few
# scenes, checking for GL errors + blank frames. Needs the root `make` + Assimp.
RENDER_OBJS = $(filter-out ../build/obj/mge_gui.o,$(wildcard ../build/obj/*.o))
render: render_smoke$(EXE)
	$(call runexe,render_smoke$(EXE))

render_smoke$(EXE): render_smoke.c $(RENDER_OBJS)
	$(CC) $(CPPFLAGS) -I../vendor/glad/include -I../vendor/stb -I../vendor/glfw/include -I$(ASSIMP)/include $(CFLAGS) -c render_smoke.c -o render_smoke.o
	$(CXX) $(CFLAGS) -o $@ $(LDFLAGS) render_smoke.o $(RENDER_OBJS) -L../vendor/glfw/lib -L../vendor/glfw/lib64 -L$(ASSIMP)/lib -L$(ASSIMP)/lib64 -lglfw3 -lassimp -lzlibstatic $(RENDER_SYSLIBS)
	-$(RM) $(call native,render_smoke.o)

test_math$(EXE): test_math.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_math.c ../source/mge_math.c -lm

test_utils$(EXE): test_utils.c ../source/mge_utils.c ../source/mge_pak.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_utils.c ../source/mge_utils.c ../source/mge_pak.c -lm

test_pak$(EXE): test_pak.c ../source/mge_pak.c ../source/mge_pak.h $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_pak.c ../source/mge_pak.c -lm

test_object$(EXE): test_object.c ../source/mge_object.c ../source/mge_math.c ../source/mge_material.c ../source/mge_physics.c ../source/mge_component.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_object.c ../source/mge_object.c ../source/mge_math.c ../source/mge_material.c ../source/mge_physics.c ../source/mge_component.c -lm

# component add/remove/get + typed accessors (hermetic: mge_material for the seed default)
test_component$(EXE): test_component.c ../source/mge_component.c ../source/mge_object.c ../source/mge_math.c ../source/mge_material.c ../source/mge_physics.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_component.c ../source/mge_component.c ../source/mge_object.c ../source/mge_math.c ../source/mge_material.c ../source/mge_physics.c -lm

test_gizmo$(EXE): test_gizmo.c ../source/mge_gizmo.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_gizmo.c ../source/mge_gizmo.c ../source/mge_math.c -lm

# raycasting + screen->ray + collider overlap + the linear rigid-body step;
# the debug-draw backend is stubbed in the test
test_physics$(EXE): test_physics.c ../source/mge_physics.c ../source/mge_body.c ../source/mge_component.c ../source/mge_material.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_physics.c ../source/mge_physics.c ../source/mge_body.c ../source/mge_component.c ../source/mge_material.c ../source/mge_math.c -lm

# editor .mgscene (de)serialiser + path helpers; the parser reads via Mge_LoadFileText (pak-aware)
# and converts rotations via the quaternion helpers (mge_math.c)
IO_ENGINE = ../source/mge_utils.c ../source/mge_pak.c ../source/mge_math.c ../source/mge_component.c ../source/mge_material.c
test_scene_io$(EXE): test_scene_io.c ../editor/scene_io.c ../editor/pathutil.c $(IO_ENGINE) ../editor/scene.h ../editor/scene_io.h $(MLIB)/test.h
	$(CC) $(CPPFLAGS) -I../editor $(CFLAGS) -o $@ $(LDFLAGS) test_scene_io.c ../editor/scene_io.c ../editor/pathutil.c $(IO_ENGINE) -lm

# editor project.mgproject (de)serialiser + Project helpers
test_project_io$(EXE): test_project_io.c ../editor/project.c ../editor/project_io.c ../editor/pathutil.c $(IO_ENGINE) ../editor/project.h ../editor/project_io.h $(MLIB)/test.h
	$(CC) $(CPPFLAGS) -I../editor $(CFLAGS) -o $@ $(LDFLAGS) test_project_io.c ../editor/project.c ../editor/project_io.c ../editor/pathutil.c $(IO_ENGINE) -lm

# dynamic library loading (mge_dylib.c) -- compiles + loads a tiny .dll/.so at runtime
test_dylib$(EXE): test_dylib.c ../source/mge_dylib.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_dylib.c ../source/mge_dylib.c -lm

test_material$(EXE): test_material.c ../source/mge_material.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_material.c ../source/mge_material.c -lm

test_light$(EXE): test_light.c ../source/mge_light.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_light.c ../source/mge_light.c -lm

test_mesh$(EXE): test_mesh.c ../source/mge_mesh.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_mesh.c ../source/mge_mesh.c -lm

test_depth$(EXE): test_depth.c ../source/mge_depth.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_depth.c ../source/mge_depth.c -lm

test_stencil$(EXE): test_stencil.c ../source/mge_stencil.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_stencil.c ../source/mge_stencil.c -lm

test_cull$(EXE): test_cull.c ../source/mge_cull.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_cull.c ../source/mge_cull.c -lm

test_framebuffer$(EXE): test_framebuffer.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_framebuffer.c -lm

test_cubemap$(EXE): test_cubemap.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_cubemap.c -lm

# mge_geometry.c needs the GL_*_SHADER constants (glad header only, no linking)
test_geometry$(EXE): test_geometry.c ../source/mge_geometry.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) -I../vendor/glad/include $(CFLAGS) -o $@ $(LDFLAGS) test_geometry.c ../source/mge_geometry.c -lm

# ModelBatch struct + CPU matrix packing; the instanced draw path is raw GL (untested here)
test_instancing$(EXE): test_instancing.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_instancing.c ../source/mge_math.c -lm

test_msaa$(EXE): test_msaa.c ../source/mge_msaa.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_msaa.c ../source/mge_msaa.c -lm

test_gamma$(EXE): test_gamma.c ../source/mge_gamma.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_gamma.c ../source/mge_gamma.c -lm

# ShadowMap struct only; the depth FBO + PCF shader need a live context
test_shadow$(EXE): test_shadow.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_shadow.c ../source/mge_math.c -lm

# the renderer backend against a fake <glad/glad.h> (test/glstub/), which records
# every GL call -- no context, no window
test_gl$(EXE): test_gl.c ../source/mge_gl.c ../source/mge_math.c glstub/glstub.c glstub/glad/glad.h glstub/glstub.h $(MLIB)/test.h
	$(CC) $(CPPFLAGS) -Iglstub $(CFLAGS) -o $@ $(LDFLAGS) test_gl.c ../source/mge_gl.c ../source/mge_math.c glstub/glstub.c -lm

test_debug$(EXE): test_debug.c ../source/mge_debug.c glstub/glstub.c glstub/glad/glad.h glstub/glstub.h $(MLIB)/test.h
	$(CC) $(CPPFLAGS) -Iglstub $(CFLAGS) -o $@ $(LDFLAGS) test_debug.c ../source/mge_debug.c glstub/glstub.c -lm

# compiled as C11 with gcc, linked with g++ (Assimp is C++)
test_model$(EXE): test_model.c ../source/mge_model.c ../source/mge_mesh.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(ASSIMP)/include -c test_model.c -o test_model_t.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(ASSIMP)/include -c ../source/mge_model.c -o test_model_m.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c ../source/mge_mesh.c -o test_model_mesh.o
	$(CXX) $(CFLAGS) -o $@ $(LDFLAGS) test_model_t.o test_model_m.o test_model_mesh.o -L$(ASSIMP)/lib -L$(ASSIMP)/lib64 -lassimp -lzlibstatic
	-$(RM) $(call native,test_model_t.o test_model_m.o test_model_mesh.o)

clean:
	-$(RM) $(call native,$(addsuffix $(EXE),$(TESTS)) test_model$(EXE) render_smoke$(EXE) _mge_utils.tmp _model_test.obj)
