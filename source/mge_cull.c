// Face culling -- thin wrappers over the GL state (mge_gl.c). Off by default;
// the engine never enables it for you, so 2D drawing is unaffected.

#include "mge.h"
#include "mge_gl.h"

void Mge_EnableFaceCulling(void) { MgeGL_SetFaceCulling(true); }
void Mge_DisableFaceCulling(void) { MgeGL_SetFaceCulling(false); }
void Mge_SetCullFace(int face) { MgeGL_SetCullFace(face); }
void Mge_SetFrontFace(int winding) { MgeGL_SetFrontFace(winding); }
