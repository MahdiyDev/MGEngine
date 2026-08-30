# Unit tests for the parts of MGEngine that need no window / GL context.
#
#   make            build and run every suite
#   make model      build and run test_model (needs `make vendor` for Assimp)
#   make clean

ifeq ($(origin CC),default)
CC := gcc
endif
CXX ?= g++

MLIB   := ../vendor/mlib
ASSIMP := ../vendor/assimp
CFLAGS ?= -std=c11 -Wall -Wextra -O1 -g
CPPFLAGS += -DPLATFORM_DESKTOP -I../source -I$(MLIB) -I$(MLIB)/vec

TESTS := test_math test_utils test_object test_material test_light test_mesh test_depth test_stencil test_cull test_framebuffer test_cubemap test_geometry test_instancing test_msaa

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
else
    EXE    :=
    RM     := rm -f
    native  = $1
    runexe  = ./$1
    LDFLAGS ?=
endif

.PHONY: all test model clean $(TESTS)

all: test
test: $(TESTS)

$(TESTS): %: %$(EXE)
	$(call runexe,$<)

# not part of `make test`: needs the vendored Assimp lib (run `make vendor`)
model: test_model$(EXE)
	$(call runexe,test_model$(EXE))

test_math$(EXE): test_math.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_math.c ../source/mge_math.c -lm

test_utils$(EXE): test_utils.c ../source/mge_utils.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_utils.c ../source/mge_utils.c

test_object$(EXE): test_object.c ../source/mge_object.c ../source/mge_math.c ../source/mge_material.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_object.c ../source/mge_object.c ../source/mge_math.c ../source/mge_material.c -lm

test_material$(EXE): test_material.c ../source/mge_material.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_material.c ../source/mge_material.c

test_light$(EXE): test_light.c ../source/mge_light.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_light.c ../source/mge_light.c -lm

test_mesh$(EXE): test_mesh.c ../source/mge_mesh.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_mesh.c ../source/mge_mesh.c

test_depth$(EXE): test_depth.c ../source/mge_depth.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_depth.c ../source/mge_depth.c

test_stencil$(EXE): test_stencil.c ../source/mge_stencil.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_stencil.c ../source/mge_stencil.c

test_cull$(EXE): test_cull.c ../source/mge_cull.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_cull.c ../source/mge_cull.c

test_framebuffer$(EXE): test_framebuffer.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_framebuffer.c

test_cubemap$(EXE): test_cubemap.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_cubemap.c

# mge_geometry.c needs the GL_*_SHADER constants (glad header only, no linking)
test_geometry$(EXE): test_geometry.c ../source/mge_geometry.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) -I../vendor/glad/include $(CFLAGS) -o $@ $(LDFLAGS) test_geometry.c ../source/mge_geometry.c

# ModelBatch struct + CPU matrix packing; the instanced draw path is raw GL (untested here)
test_instancing$(EXE): test_instancing.c ../source/mge_math.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_instancing.c ../source/mge_math.c -lm

test_msaa$(EXE): test_msaa.c ../source/mge_msaa.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(LDFLAGS) test_msaa.c ../source/mge_msaa.c

# compiled as C11 with gcc, linked with g++ (Assimp is C++)
test_model$(EXE): test_model.c ../source/mge_model.c ../source/mge_mesh.c $(MLIB)/test.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(ASSIMP)/include -c test_model.c -o test_model_t.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(ASSIMP)/include -c ../source/mge_model.c -o test_model_m.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -c ../source/mge_mesh.c -o test_model_mesh.o
	$(CXX) $(CFLAGS) -o $@ $(LDFLAGS) test_model_t.o test_model_m.o test_model_mesh.o -L$(ASSIMP)/lib -lassimp -lzlibstatic
	-$(RM) $(call native,test_model_t.o test_model_m.o test_model_mesh.o)

clean:
	-$(RM) $(call native,$(addsuffix $(EXE),$(TESTS)) test_model$(EXE) _mge_utils.tmp _model_test.obj)
