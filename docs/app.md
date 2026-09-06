# Windowing, input & the frame loop

## GL debug output

The driver can report invalid API use, undefined behaviour and performance
warnings through a callback the instant they happen — so a broken draw is a loud
`GL DEBUG [...]` log line, not a silently wrong frame.

```c
Mge_SetDebugOutput(false);   // before Mge_InitWindow; default: on unless built -DNDEBUG
```

On in normal builds, off in `make release`. It requests a debug GL context and
registers a synchronous `glDebugMessageCallback`; the `SEVERITY_NOTIFICATION`
chatter is muted. It catches *invalid* GL, not *valid-but-wrong* rendering — a
screenshot check is what catches that.

## Cursor

| Function | Effect |
| --- | --- |
| `ShowCursor()` / `HideCursor()` | toggle cursor **visibility** (the cursor still moves freely) |
| `EnableCursor()` | show **and** unlock the cursor |
| `DisableCursor()` | hide **and** lock the cursor to the window centre (FPS style, enables raw mouse motion) |
| `Mge_ToggleCursor()` | flip between `EnableCursor()` and `DisableCursor()` |
| `IsCursorHidden()` | `true` while the cursor is hidden / locked |

Bind it to a key in your loop — `editor/main.c` uses **TAB** to free and re-lock
the mouse, and only runs the fly-camera while it is locked:

```c
Mge_InitWindow(1280, 720, "demo");
DisableCursor();                        // start with a captured cursor

while (!Mge_WindowShouldClose()) {
    Mge_BeginDrawing();

    if (IsKeyPressed(KEY_TAB))
        Mge_ToggleCursor();

    if (IsCursorHidden())
        update_camera(&camera);         // mouse-look only while captured

    /* ... draw ... */
    Mge_EndDrawing();
}
```

## A resizable window

The window is a fixed size by default. Call `Mge_SetWindowResizable(true)` **before**
`Mge_InitWindow` to let the OS resize it (min 640x400); `Mge_SetWindowSize(w, h)`
resizes it from code; `Mge_ToggleFullscreen()` flips borderless fullscreen on the
primary monitor and back (`Mge_IsFullscreen()` reports it; the editor and player
bind it to **F11**). Either way `Mge_GetScreenWidth/Height` and the GL viewport
follow the window, so 2D layout and `Mge_BeginMode3D`'s aspect stay correct.
Anything **you** sized to the framebuffer — a `RenderTexture` for a post-fx pass,
a `BloomFX` — must be recreated when the size changes (`editor/main.c` compares
the size each frame and calls `Scene_Resize`). `editor/prefs.c` shows persisting
the window size across runs.

## Frame pacing

`Mge_SetTargetFPS(fps)` caps the loop (a spin/sleep limiter; 0 = uncapped).
`Mge_SetVSync(true)` (call after `Mge_InitWindow`) syncs buffer swaps to the
display instead — no tearing, and the setting survives a fullscreen toggle.
`Mge_GetMonitorRefreshRate()` returns the primary monitor's Hz (0 if unknown);
the editor and player use `Mge_SetVSync(true)` plus `Mge_SetTargetFPS(hz)` as a
fallback cap.

## Screenshots

`Mge_TakeScreenshot("shot.png")` reads the window framebuffer back and writes a
PNG (rows flipped to top-down). Call it after drawing and before the buffers swap
— the end of the loop body, after `Mge_GuiEndFrame` but before (or right after)
`Mge_EndDrawing`. `MgeGL_SaveScreenshot(path, x, y, w, h)` is the lower-level
form that captures an arbitrary rectangle. `editor/main.c` binds **F12** to it.

```c
if (IsKeyPressed(KEY_F12))
    Mge_TakeScreenshot("screenshot.png");   // next to the executable
```

