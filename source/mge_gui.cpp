// Dear ImGui backend for the abstract UI in mge_gui.h. This is the only C++
// translation unit in the engine; everything it exposes has C linkage.

#include "mge_gui.h"
#include "mge_gl.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cfloat>

static bool s_ready = false;   // backend initialised
static bool s_inFrame = false; // between BeginFrame / EndFrame

static void EnsureInit(void)
{
    if (s_ready)
        return;

    void* window = Mge_GetWindowHandle();
    if (window == nullptr)
        return; // Mge_InitWindow hasn't run yet

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().IniFilename = nullptr; // don't litter an imgui.ini

    // `true` -> install GLFW callbacks, chaining the ones the engine already set
    ImGui_ImplGlfw_InitForOpenGL(reinterpret_cast<GLFWwindow*>(window), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    s_ready = true;
}

extern "C" {

void Mge_GuiBeginFrame(void)
{
    EnsureInit();
    if (!s_ready)
        return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    s_inFrame = true;
}

void Mge_GuiEndFrame(void)
{
    if (!s_inFrame)
        return;

    MgeGL_Draw(); // flush the engine's batch before ImGui touches GL state

    // ImGui's vertex colours are already display-space; don't let
    // GL_FRAMEBUFFER_SRGB re-encode them. Drop it for the UI pass, restore after.
    const bool srgb = Mge_GetGammaCorrection();
    if (srgb)
        MgeGL_SetFramebufferSRGB(false);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (srgb)
        MgeGL_SetFramebufferSRGB(true);

    s_inFrame = false;
}

void Mge_GuiShutdown(void)
{
    if (!s_ready)
        return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    s_ready = false;
}

bool Mge_GuiWantsMouse(void) { return s_ready && ImGui::GetIO().WantCaptureMouse; }
bool Mge_GuiWantsKeyboard(void) { return s_ready && ImGui::GetIO().WantCaptureKeyboard; }

// --- containers ---

bool Mge_GuiBeginBox(const char* title, float x, float y, float w, float h)
{
    if (!s_inFrame)
        return false;
    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
    if (w > 0.0f && h > 0.0f)
        ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_FirstUseEver);
    return ImGui::Begin(title);
}

void Mge_GuiEndBox(void)
{
    if (s_inFrame)
        ImGui::End();
}

bool Mge_GuiBeginSidebar(const char* title, float width, bool rightEdge)
{
    if (!s_inFrame)
        return false;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    float x = rightEdge ? (vp->WorkPos.x + vp->WorkSize.x - width) : vp->WorkPos.x;
    ImGui::SetNextWindowPos(ImVec2(x, vp->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(width, vp->WorkSize.y));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
    return ImGui::Begin(title, nullptr, flags);
}

void Mge_GuiEndSidebar(void)
{
    if (s_inFrame)
        ImGui::End();
}

bool Mge_GuiBeginPanel(const char* title, float x, float y, float w, float h)
{
    if (!s_inFrame)
        return false;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + x, vp->WorkPos.y + y));
    ImGui::SetNextWindowSize(ImVec2(w, h));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoTitleBar;
    return ImGui::Begin(title, nullptr, flags);
}

void Mge_GuiEndPanel(void)
{
    if (s_inFrame)
        ImGui::End();
}

// --- widgets ---

void Mge_GuiLabel(const char* text)
{
    if (s_inFrame)
        ImGui::TextUnformatted(text);
}
void Mge_GuiSeparator(void)
{
    if (s_inFrame)
        ImGui::Separator();
}
void Mge_GuiSpacing(void)
{
    if (s_inFrame)
        ImGui::Spacing();
}
void Mge_GuiSameLine(void)
{
    if (s_inFrame)
        ImGui::SameLine();
}
void Mge_GuiSetNextItemWidth(float width)
{
    if (s_inFrame)
        ImGui::SetNextItemWidth(width > 0.0f ? width : -FLT_MIN);
}
bool Mge_GuiButton(const char* label) { return s_inFrame && ImGui::Button(label); }
bool Mge_GuiSelectable(const char* label, bool selected)
{
    return s_inFrame && ImGui::Selectable(label, selected);
}
bool Mge_GuiSelectableEx(const char* label, bool selected, bool* doubleClicked)
{
    if (!s_inFrame)
        return false;
    bool clicked = ImGui::Selectable(label, selected, ImGuiSelectableFlags_AllowDoubleClick);
    if (doubleClicked != nullptr)
        *doubleClicked = clicked && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    return clicked;
}

bool Mge_GuiInputText(const char* label, char* buf, int bufSize)
{
    if (!s_inFrame || buf == nullptr || bufSize <= 0)
        return false;
    return ImGui::InputText(label, buf, (size_t)bufSize);
}

void Mge_GuiLogBox(const char* id, const char* text)
{
    if (!s_inFrame)
        return;
    if (text == nullptr)
        text = "";

    ImGui::PushID(id);
    if (ImGui::BeginChild("##logbox", ImVec2(0, 0), true,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleVar();
        // stick to the bottom while new output arrives
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::PopID();
}

bool Mge_GuiBeginMenu(const char* label)
{
    if (!s_inFrame)
        return false;
    ImGui::PushID(label);
    if (ImGui::Button(label))
        ImGui::OpenPopup("##menu");
    bool open = ImGui::BeginPopup("##menu");
    if (!open)
        ImGui::PopID();
    return open;
}
bool Mge_GuiMenuItem(const char* label)
{
    return s_inFrame && ImGui::MenuItem(label);
}
void Mge_GuiEndMenu(void)
{
    if (!s_inFrame)
        return;
    ImGui::EndPopup();
    ImGui::PopID();
}

void Mge_GuiOpenPopup(const char* id)
{
    if (s_inFrame)
        ImGui::OpenPopup(id);
}
bool Mge_GuiBeginPopup(const char* id)
{
    if (!s_inFrame)
        return false;
    return ImGui::BeginPopupModal(id, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
}
void Mge_GuiEndPopup(void)
{
    if (s_inFrame)
        ImGui::EndPopup();
}
void Mge_GuiClosePopup(void)
{
    if (s_inFrame)
        ImGui::CloseCurrentPopup();
}

bool Mge_GuiImageButton(const char* strId, unsigned int textureId, float size)
{
    if (!s_inFrame)
        return false;

    const ImVec2 box(size, size);
    ImGui::PushID(strId);
    bool clicked = (textureId == 0)
        ? ImGui::Button("+", box)
        : ImGui::ImageButton("img", (ImTextureID)(intptr_t)textureId, box);
    ImGui::PopID();
    return clicked;
}

bool Mge_GuiCheckbox(const char* label, bool* value)
{
    return s_inFrame && ImGui::Checkbox(label, value);
}
bool Mge_GuiCombo(const char* label, int* index, const char* const* items, int count)
{
    return s_inFrame && ImGui::Combo(label, index, items, count);
}
bool Mge_GuiInputInt(const char* label, int* value)
{
    return s_inFrame && ImGui::InputInt(label, value);
}
bool Mge_GuiInputFloat(const char* label, float* value)
{
    return s_inFrame && ImGui::DragFloat(label, value, 0.05f);
}
bool Mge_GuiSliderFloat(const char* label, float* value, float min, float max)
{
    return s_inFrame && ImGui::SliderFloat(label, value, min, max);
}

bool Mge_GuiInputVec2(const char* label, Vector2* value)
{
    if (!s_inFrame)
        return false;
    float v[2] = { value->x, value->y };
    if (ImGui::DragFloat2(label, v, 0.05f)) {
        value->x = v[0];
        value->y = v[1];
        return true;
    }
    return false;
}

bool Mge_GuiInputVec3(const char* label, Vector3* value)
{
    if (!s_inFrame)
        return false;
    float v[3] = { value->x, value->y, value->z };
    if (ImGui::DragFloat3(label, v, 0.05f)) {
        value->x = v[0];
        value->y = v[1];
        value->z = v[2];
        return true;
    }
    return false;
}

bool Mge_GuiInputColor(const char* label, Color* value)
{
    if (!s_inFrame)
        return false;
    float c[4] = { value->r / 255.0f, value->g / 255.0f, value->b / 255.0f, value->a / 255.0f };
    if (ImGui::ColorEdit4(label, c)) {
        value->r = (unsigned char)(c[0] * 255.0f + 0.5f);
        value->g = (unsigned char)(c[1] * 255.0f + 0.5f);
        value->b = (unsigned char)(c[2] * 255.0f + 0.5f);
        value->a = (unsigned char)(c[3] * 255.0f + 0.5f);
        return true;
    }
    return false;
}

bool Mge_GuiInputColorRGB(const char* label, Vector3* value)
{
    if (!s_inFrame)
        return false;
    float c[3] = { value->x, value->y, value->z };
    if (ImGui::ColorEdit3(label, c)) {
        value->x = c[0];
        value->y = c[1];
        value->z = c[2];
        return true;
    }
    return false;
}

} // extern "C"
