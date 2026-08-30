// OpenGL debug output (KHR_debug, core since GL 4.3).
//
// When on, the driver reports invalid API use, undefined behaviour and
// performance warnings through a callback the moment they happen -- so a broken
// draw call is a loud log line instead of a silently wrong frame. It catches
// *invalid* GL, not *logically wrong but valid* rendering (a screenshot test is
// what catches that).
//
// Default: on in debug builds, off when compiled with -DNDEBUG (the `release`
// target). Call Mge_SetDebugOutput before Mge_InitWindow to override.

#include "mge.h"
#include "mge_utils.h"

#include <glad/glad.h>

#ifdef NDEBUG
static bool s_enabled = false;
#else
static bool s_enabled = true;
#endif

void Mge_SetDebugOutput(bool enabled)
{
    s_enabled = enabled; // takes effect at the next Mge_InitWindow
}

bool Mge_GetDebugOutput(void)
{
    return s_enabled;
}

static void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
    GLsizei length, const GLchar* message, const void* user)
{
    (void)type;
    (void)length;
    (void)user;
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    const char* sev = (severity == GL_DEBUG_SEVERITY_HIGH)     ? "HIGH"
                      : (severity == GL_DEBUG_SEVERITY_MEDIUM) ? "MEDIUM"
                                                              : "LOW";
    const char* src = (source == GL_DEBUG_SOURCE_SHADER_COMPILER) ? "shader"
                      : (source == GL_DEBUG_SOURCE_API)           ? "api"
                      : (source == GL_DEBUG_SOURCE_WINDOW_SYSTEM) ? "wsi"
                                                                 : "other";
    int level = (severity == GL_DEBUG_SEVERITY_HIGH) ? LOG_ERROR : LOG_WARNING;
    TRACE_LOG(level, "GL DEBUG [%s] %s (id %u): %s", sev, src, id, message);
}

void Mge_ApplyDebugOutput(void)
{
    if (!s_enabled)
        return;
    if (glDebugMessageCallback == NULL) // driver didn't give us a debug context
        return;

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // callback fires on the offending call, with a usable stack
    glDebugMessageCallback(DebugCallback, NULL);
    // silence the low-priority "buffer will use VIDEO memory" chatter
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
    TRACE_LOG(LOG_INFO, "GL: debug output enabled");
}
