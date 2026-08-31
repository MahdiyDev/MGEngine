// Native "open file" dialog.
//
//   Windows -> GetOpenFileName (comdlg32)
//   Linux   -> zenity, then kdialog (whichever is on PATH), via popen
//
// Returns a malloc'd absolute path the caller frees, or NULL on cancel / when no
// backend is available. The dialog never changes the process working directory
// (OFN_NOCHANGEDIR / an absolute-path request), so relative asset paths keep
// working afterwards.

// No "mge.h" here: on Windows <windows.h> collides with the engine's `Rectangle`
// / `ShowCursor` names. These prototypes are the whole public surface.
char* Mge_OpenFileDialog(const char* title, const char* filterName, const char* filterExts);
char* Mge_OpenImageDialog(void);
char* Mge_SaveFileDialog(const char* title, const char* filterName, const char* filterExts,
    const char* defaultName);

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

char* Mge_OpenFileDialog(const char* title, const char* filterName, const char* filterExts)
{
    // filter string is "Name\0*.ext;*.ext\0All files\0*.*\0\0"
    char filter[256];
    int n = 0;
    const char* name = (filterName != NULL) ? filterName : "Files";
    const char* exts = (filterExts != NULL) ? filterExts : "*.*";
    n += snprintf(filter + n, sizeof(filter) - n, "%s", name) + 1;
    n += snprintf(filter + n, sizeof(filter) - n, "%s", exts) + 1;
    n += snprintf(filter + n, sizeof(filter) - n, "All files") + 1;
    n += snprintf(filter + n, sizeof(filter) - n, "*.*") + 1;
    filter[n] = '\0';

    char path[MAX_PATH] = { 0 };

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

    if (!GetOpenFileNameA(&ofn) || path[0] == '\0')
        return NULL;

    return _strdup(path);
}

char* Mge_SaveFileDialog(const char* title, const char* filterName, const char* filterExts,
    const char* defaultName)
{
    char filter[256];
    int n = 0;
    const char* fname = (filterName != NULL) ? filterName : "Files";
    const char* exts = (filterExts != NULL) ? filterExts : "*.*";
    n += snprintf(filter + n, sizeof(filter) - n, "%s", fname) + 1;
    n += snprintf(filter + n, sizeof(filter) - n, "%s", exts) + 1;
    n += snprintf(filter + n, sizeof(filter) - n, "All files") + 1;
    n += snprintf(filter + n, sizeof(filter) - n, "*.*") + 1;
    filter[n] = '\0';

    char path[MAX_PATH] = { 0 };
    if (defaultName != NULL)
        snprintf(path, sizeof(path), "%s", defaultName);

    OPENFILENAMEA ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameA(&ofn) || path[0] == '\0')
        return NULL;

    return _strdup(path);
}

#else // POSIX: shell out to a desktop file picker

#include <stdio.h>

static char* run_picker(const char* cmd)
{
    FILE* p = popen(cmd, "r");
    if (p == NULL)
        return NULL;

    char buf[4096] = { 0 };
    size_t got = fread(buf, 1, sizeof(buf) - 1, p);
    int rc = pclose(p);
    if (rc != 0 || got == 0)
        return NULL;

    buf[got] = '\0';
    buf[strcspn(buf, "\r\n")] = '\0'; // trim the trailing newline
    if (buf[0] == '\0')
        return NULL;

    return strdup(buf);
}

char* Mge_OpenFileDialog(const char* title, const char* filterName, const char* filterExts)
{
    char cmd[512];
    const char* t = (title != NULL) ? title : "Open File";

    // canonical filter form is ';'-separated (Windows); zenity wants spaces
    char globs[256] = { 0 };
    if (filterExts != NULL) {
        snprintf(globs, sizeof(globs), "%s", filterExts);
        for (char* c = globs; *c != '\0'; c++)
            if (*c == ';')
                *c = ' ';
    }

    // zenity: --file-filter "Name | *.a *.b"
    if (system("command -v zenity >/dev/null 2>&1") == 0) {
        if (filterExts != NULL)
            snprintf(cmd, sizeof(cmd),
                "zenity --file-selection --title=\"%s\" --file-filter=\"%s | %s\" 2>/dev/null",
                t, (filterName != NULL) ? filterName : "Files", globs);
        else
            snprintf(cmd, sizeof(cmd), "zenity --file-selection --title=\"%s\" 2>/dev/null", t);
        char* r = run_picker(cmd);
        if (r != NULL)
            return r;
    }

    if (system("command -v kdialog >/dev/null 2>&1") == 0) {
        snprintf(cmd, sizeof(cmd), "kdialog --getopenfilename . --title \"%s\" 2>/dev/null", t);
        return run_picker(cmd);
    }

    (void)filterName;
    return NULL;
}

char* Mge_SaveFileDialog(const char* title, const char* filterName, const char* filterExts,
    const char* defaultName)
{
    char cmd[512];
    const char* t = (title != NULL) ? title : "Save File";
    (void)filterName;
    (void)filterExts;

    if (system("command -v zenity >/dev/null 2>&1") == 0) {
        snprintf(cmd, sizeof(cmd),
            "zenity --file-selection --save --confirm-overwrite --title=\"%s\" --filename=\"%s\" 2>/dev/null",
            t, (defaultName != NULL) ? defaultName : "");
        char* r = run_picker(cmd);
        if (r != NULL)
            return r;
    }
    if (system("command -v kdialog >/dev/null 2>&1") == 0) {
        snprintf(cmd, sizeof(cmd), "kdialog --getsavefilename \"%s\" --title \"%s\" 2>/dev/null",
            (defaultName != NULL) ? defaultName : ".", t);
        return run_picker(cmd);
    }
    return NULL;
}

#endif

char* Mge_OpenImageDialog(void)
{
    return Mge_OpenFileDialog("Open image",
        "Images", "*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif;*.psd");
}
