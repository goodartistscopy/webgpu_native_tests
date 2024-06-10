#pragma once

#include <string>
#include <vector>

#include <imgui/imgui.h>
#include <webgpu/webgpu_cpp.h>

class ImGuiWebGPU
{
public:
    ImGuiWebGPU(wgpu::Device device);

    void begin_frame(uint32_t win_width, uint32_t win_height);

    void end_frame(wgpu::Texture target);

    /* void init(size_t winWidth, size_t winHeight, Chameleon::ScenePtr scene); */

    /* void resize(size_t winWidth, size_t winHeight); */

    /* /// */
    /* ~ImGuiHelper(); */

    /* /// */
    /* ImGuiContext* getContext() const; */

    /* void setModifiers(int mods) const; */

    /* void setKeyState(UI::Key::Enum key, UI::Action::Enum action, int mods) const; */

    /* void setCharacter(unsigned int c) const; */

    /* void setMousePosition(float x, float y); */

    /* void setButtonState(UI::Mouse::Enum button, UI::Action::Enum action, int mods); */

    /* void setWheelScroll(double amount); */

    /* /// Start a new Imgui frame */
    /* void begin(); */

    /* /// End the ImGui frame */
    /* void end(); */

    /* static ImTextureID createTextureId(const std::string& id); */

private:
    /* static const std::string s_fontTextureName; */
    /* static const std::string s_baseMeshName; */

    ImGuiContext *context;
    wgpu::Device device;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bind_groups[2];
    wgpu::Buffer view_uniform_buffer;
    wgpu::Buffer draw_flags_uniform_buffer;
    wgpu::Texture font_texture;
    uint32_t last_draw_flags;
    wgpu::Texture* last_texture;
    /* std::vector<std::string> m_meshes; */
    /* std::vector<std::string> m_entities; */

    /* float m_lastScroll = 0; */
};
