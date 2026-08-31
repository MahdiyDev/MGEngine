#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void History_Init(History* h)
{
    memset(h, 0, sizeof(*h));
    h->undo = calloc(HISTORY_MAX, sizeof(Scene));
    h->redo = calloc(HISTORY_MAX, sizeof(Scene));
}

void History_Free(History* h)
{
    free(h->undo);
    free(h->redo);
    memset(h, 0, sizeof(*h));
}

void History_SetRoot(History* h, const char* root)
{
    snprintf(h->root, sizeof(h->root), "%s", (root != NULL) ? root : "");
}

void History_Reset(History* h)
{
    h->undoCount = 0;
    h->redoCount = 0;
    h->haveStaged = false;
    h->burst = false;
    h->touched = false;
}

// drop the oldest entry so a fresh one fits at the top
static void push(Scene* stack, int* count, const Scene* s)
{
    if (*count == HISTORY_MAX) {
        memmove(&stack[0], &stack[1], sizeof(Scene) * (HISTORY_MAX - 1));
        (*count)--;
    }
    stack[(*count)++] = *s;
}

void History_Rest(History* h, const Scene* s)
{
    h->staged = *s;
    h->haveStaged = true;
    h->burst = false;
}

void History_Record(History* h)
{
    h->touched = true;
    if (h->burst || !h->haveStaged)
        return; // already captured this burst (or nothing to capture yet)
    push(h->undo, &h->undoCount, &h->staged);
    h->redoCount = 0;
    h->burst = true;
}

void History_EndFrame(History* h)
{
    h->touched = false;
}

bool History_CanUndo(const History* h) { return h->undoCount > 0; }
bool History_CanRedo(const History* h) { return h->redoCount > 0; }

bool History_Undo(History* h, Scene* s)
{
    if (h->undoCount == 0)
        return false;
    push(h->redo, &h->redoCount, s);
    Scene snap = h->undo[--h->undoCount];
    Scene_RestoreSnapshot(s, &snap, h->root);
    h->staged = *s;
    h->haveStaged = true;
    h->burst = false;
    return true;
}

bool History_Redo(History* h, Scene* s)
{
    if (h->redoCount == 0)
        return false;
    push(h->undo, &h->undoCount, s);
    Scene snap = h->redo[--h->redoCount];
    Scene_RestoreSnapshot(s, &snap, h->root);
    h->staged = *s;
    h->haveStaged = true;
    h->burst = false;
    return true;
}
