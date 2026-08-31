// Undo / redo for the editor. The unit of history is a whole-Scene snapshot:
// every undoable action (transform edits, add / delete / rename / duplicate /
// reorder / primitive change, material tweaks) is captured by comparing against
// a "staged" baseline that is refreshed whenever the scene is at rest.
//
//   each frame, when nothing is being edited:  History_Rest(&h, &scene)
//   at every mutation site:                     History_Record(&h)
//   Ctrl+Z / Ctrl+Y:                            History_Undo / History_Redo
#pragma once

#include "scene.h"

#define HISTORY_MAX 48

typedef struct History {
    Scene* undo; // heap arrays; oldest entry drops when full
    Scene* redo;
    int undoCount;
    int redoCount;

    Scene staged;    // last at-rest scene; pushed on the first Record of a burst
    bool haveStaged;
    bool burst;      // a Record burst is open (coalesces per-frame gizmo drags)
    bool touched;    // Record was called this frame -> main.c defers History_Rest

    char root[512];  // project root -> texture / skybox reload on restore
} History;

void History_Init(History* h);
void History_Free(History* h);
void History_SetRoot(History* h, const char* root);

// Drop all history (undo + redo + baseline). Call when the active scene is
// replaced wholesale -- new / open / add / switch / revert.
void History_Reset(History* h);

// Call once per frame when the scene is idle: not `h->touched`, no gizmo drag, no
// focused text field. Refreshes the baseline and closes any open burst.
void History_Rest(History* h, const Scene* s);

// Call at a mutation site. Pushes the staged baseline once per burst; clears
// redo. Sets `h->touched` so the caller skips History_Rest this frame.
void History_Record(History* h);

// Clear `h->touched` -- main.c calls this at the very end of each frame.
void History_EndFrame(History* h);

bool History_CanUndo(const History* h);
bool History_CanRedo(const History* h);

// Restore the scene one step back / forward. Returns true if it changed anything.
bool History_Undo(History* h, Scene* s);
bool History_Redo(History* h, Scene* s);
