// Immediate-mode UI, abstracted over Dear ImGui.
//
// Draw the UI once per frame, after the 3D/2D scene and before Mge_EndDrawing:
//
//   Mge_GuiBeginFrame();
//     if (Mge_GuiBeginSidebar("Scene", 280, false)) {
//         if (Mge_GuiSelectable("Cube 0", selected == 0)) selected = 0;
//         Mge_GuiSeparator();
//         Mge_GuiInputVec3("position", &obj.position);
//         Mge_GuiInputColor("color", &obj.color);
//     }
//     Mge_GuiEndSidebar();
//   Mge_GuiEndFrame();
//
// The engine links its GUI backend into libmgengine; nothing extra to build.

#pragma once

#include "mge.h"

#ifdef __cplusplus
extern "C" {
#endif

void Mge_GuiBeginFrame(void); // start a UI frame (lazily boots the backend)
void Mge_GuiEndFrame(void);   // render the UI on top of the current framebuffer
void Mge_GuiShutdown(void);   // optional; the OS reclaims everything at exit anyway

// Gate your own picking / camera on these -- true while a widget has focus.
bool Mge_GuiWantsMouse(void);
bool Mge_GuiWantsKeyboard(void);

// --- containers ("boxes") ---
// Begin* returns false when the box is collapsed; still call the matching End*.
bool Mge_GuiBeginBox(const char* title, float x, float y, float w, float h); // floating panel
void Mge_GuiEndBox(void);
bool Mge_GuiBeginSidebar(const char* title, float width, bool rightEdge);    // full-height dock
void Mge_GuiEndSidebar(void);

// A panel pinned to an exact screen rectangle: no title bar, move, resize or
// collapse, and it never comes to the front on click. `title` is just the
// internal id (make it unique). The caller owns the layout maths (screen size is
// Mge_GetScreenWidth/Height) -- use it to build a docked editor shell (top bar +
// left / right / bottom panels around a central viewport).
bool Mge_GuiBeginPanel(const char* title, float x, float y, float w, float h);
void Mge_GuiEndPanel(void);

// --- widgets ---
void Mge_GuiLabel(const char* text);
void Mge_GuiSeparator(void);
void Mge_GuiSpacing(void);
void Mge_GuiSameLine(void);                                // lay the next widget beside this one
void Mge_GuiSetNextItemWidth(float width);                 // width for the next input / combo (<=0 resets)
bool Mge_GuiButton(const char* label);                    // true the frame it is clicked
bool Mge_GuiSelectable(const char* label, bool selected);  // list row; true when clicked
// Like Mge_GuiSelectable, but also reports a double-click on the row (for
// rename-in-place). `doubleClicked` may be NULL.
bool Mge_GuiSelectableEx(const char* label, bool selected, bool* doubleClicked);

// A single-line text field. Writes into `buf` (NUL-terminated, <= bufSize).
// Returns true the frame the text changes.
bool Mge_GuiInputText(const char* label, char* buf, int bufSize);

// A button that opens a popup menu. Wrap the items between Begin/End; BeginMenu
// returns true only while the popup is open. Mge_GuiMenuItem is a clickable row.
//   if (Mge_GuiBeginMenu("+ add")) {
//       if (Mge_GuiMenuItem("Cube"))  ...;
//       Mge_GuiEndMenu();
//   }
bool Mge_GuiBeginMenu(const char* label);
bool Mge_GuiMenuItem(const char* label);
void Mge_GuiEndMenu(void);

// Modal popups. Call Mge_GuiOpenPopup(id) once to trigger; then every frame call
// Mge_GuiBeginPopup(id) -- it returns true only while open, wrap the body and
// call Mge_GuiEndPopup. Mge_GuiClosePopup() dismisses it from inside.
//   if (savePressed) Mge_GuiOpenPopup("confirm");
//   if (Mge_GuiBeginPopup("confirm")) { ...; if (ok) Mge_GuiClosePopup(); Mge_GuiEndPopup(); }
void Mge_GuiOpenPopup(const char* id);
bool Mge_GuiBeginPopup(const char* id);
void Mge_GuiEndPopup(void);
void Mge_GuiClosePopup(void);

// A square thumbnail button. `textureId` is a GL texture id (Texture2D.id); pass
// 0 for an empty slot (draws a bordered "+" placeholder). True the frame it is
// clicked. `strId` must be unique among sibling widgets.
bool Mge_GuiImageButton(const char* strId, unsigned int textureId, float size);

// --- inputs -- each returns true the frame its value changes ---
bool Mge_GuiCheckbox(const char* label, bool* value);
bool Mge_GuiCombo(const char* label, int* index, const char* const* items, int count); // dropdown
bool Mge_GuiInputInt(const char* label, int* value);
bool Mge_GuiInputFloat(const char* label, float* value);
bool Mge_GuiSliderFloat(const char* label, float* value, float min, float max);
bool Mge_GuiInputVec2(const char* label, Vector2* value);
bool Mge_GuiInputVec3(const char* label, Vector3* value);
bool Mge_GuiInputColor(const char* label, Color* value);        // 8-bit RGBA swatch
bool Mge_GuiInputColorRGB(const char* label, Vector3* value);   // 0..1 linear RGB (e.g. Light.color)

#ifdef __cplusplus
}
#endif
