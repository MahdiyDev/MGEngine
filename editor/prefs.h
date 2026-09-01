// Small, persistent editor preferences -- window size + panel split positions.
// Stored as `key=value` lines in `~/.mgeeditor.ini` (or the CWD if there is no
// home directory). Nothing here is critical; a missing / broken file just yields
// the defaults.
#pragma once

typedef struct EditorPrefs {
    int   winW, winH;               // last window size
    float leftW, rightW, bottomH;   // panel split positions (px)
} EditorPrefs;

// Fill `p` with defaults, then override from the ini file if it exists.
void Prefs_Load(EditorPrefs* p);

// Write `p` back to the ini file (best effort).
void Prefs_Save(const EditorPrefs* p);
