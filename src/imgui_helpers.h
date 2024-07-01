#pragma once

#include "GLFW/glfw3.h"
#include "webgpu/webgpu_glfw.h"
#include <string>
#include <vector>

#include <imgui/imgui.h>
#include <webgpu/webgpu_cpp.h>

class ImGuiWebGPU {
public:
    ImGuiWebGPU(wgpu::Device device);

    ~ImGuiWebGPU();

    void begin_frame(uint32_t win_width, uint32_t win_height);

    void end_frame(wgpu::Texture target);

private:
    ImGuiContext *context;
    wgpu::Device device;
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bind_groups[3];
    wgpu::Buffer view_uniform_buffer;
    wgpu::Buffer draw_flags_uniform_buffer;
    wgpu::Texture font_texture;
    uint32_t last_draw_flags;
    wgpu::Texture* last_texture;
};

class ImGuiGlfw {
public:
    ImGuiGlfw(GLFWwindow * window);
    ~ImGuiGlfw();

    bool is_idle();

private:
    void register_callbacks(GLFWwindow * window);
    void unregister_callbacks(GLFWwindow * window);

    static void window_size_cb(GLFWwindow* window, int width, int height);
    static void	mouse_button_cb(GLFWwindow *window, int button, int action, int mods);
    static void	cursor_pos_cb(GLFWwindow *window, double xpos, double ypos);
    static void	cursor_enter_cb(GLFWwindow *window, int entered);
    static void	scroll_cb(GLFWwindow *window, double xoffset, double yoffset);
    static void	key_cb(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void	char_cb(GLFWwindow *window, unsigned int codepoint);
    static void char_mods_cb(GLFWwindow *window, unsigned int codepoint, int mods);

    GLFWwindowsizefun prev_window_size_cb;
    GLFWmousebuttonfun prev_mouse_button_cb;
    GLFWcursorposfun prev_cursor_pos_cb;
    GLFWcursorenterfun prev_cursor_enter_cb;
    GLFWscrollfun prev_scroll_cb;
    GLFWkeyfun prev_key_cb;
    GLFWcharfun prev_char_cb;
    GLFWcharmodsfun prev_char_mods_cb;

    void * prev_window_pointer;
    GLFWwindow * window;
    int refresh_frame_count = 0;
};
