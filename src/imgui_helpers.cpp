#include "imgui_helpers.h"

/* #include <algorithm> */
/* #include <unordered_map> */
#include <_types/_uint16_t.h>
#include <_types/_uint32_t.h>
#include <cstring>
#include <iostream>
#include <string_view>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "webgpu_helpers.h"

using namespace wgpu;

/* using namespace UI; */

/* std::string const ImGuiHelper::s_fontTextureName = "_imGuiFont"; */
/* std::string const ImGuiHelper::s_baseMeshName    = "_imGuiMesh_"; */

/* ImGuiKey convertToImGuiKey(UI::Key::Enum key); */

ImGuiWebGPU::ImGuiWebGPU(Device device) {
    this->context = ImGui::CreateContext();
    this->device = device;

    ImGuiIO& io = ImGui::GetIO();

    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.IniFilename = nullptr;

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsClassic(&style);
    style.FrameRounding    = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowPadding    = ImVec2(10.0f, 10.0f);

    style.AntiAliasedLines       = true;
    style.AntiAliasedLinesUseTex = false;

    // Create pipeline
    VertexAttribute attributes[3] = {
        { .format = VertexFormat::Float32x2, .offset = 0,  .shaderLocation = 0 },
        { .format = VertexFormat::Float32x2, .offset = 8,  .shaderLocation = 1 },
        { .format = VertexFormat::Unorm8x4,  .offset = 16, .shaderLocation = 2 },
    };
    VertexBufferLayout vertex_layout{
        .arrayStride = sizeof(ImDrawVert),
        .attributeCount = 3,
        .attributes = attributes,
    };

    const char* ui_source = R"WGSL(
        struct Vertex {
            @location(0) ss_pos: vec2f,
            @location(1) uv: vec2f,
            @location(2) color: vec4f,
        };

        struct VertexOutput {
            @builtin(position) position: vec4f,
            @location(0) uv: vec2f,
            @location(1) color: vec4f,
        };

        struct DrawInfo {
            tex_is_srgb: u32,
            target_is_srgb: u32,
        };

        // (width, height, 1.0/width, 1.0/height)
        @group(0) @binding(0) var<uniform> view_info: vec4f;
        // draw flags: bit 0: texture_is_srgb
        //             bit 1: target_is_srgb
        @group(0) @binding(1) var<uniform> draw_flags : u32;

        @vertex fn vertexMain(vertex: Vertex) -> VertexOutput {
            var out: VertexOutput;

            var cs_pos = 2.0 * (vertex.ss_pos * view_info.zw) - 1.0;
            cs_pos.y = -cs_pos.y;
            out.position = vec4f(cs_pos, 0.0, 1.0);
            out.uv = vertex.uv;
            out.color = vertex.color;

            return out;
        }

        @group(0) @binding(2) var draw_sampler: sampler;
        @group(1) @binding(0) var draw_texture: texture_2d<f32>;

        struct FragmentInput {
            @builtin(position) position: vec4f,
            @location(0) uv: vec2f,
            @location(1) color: vec4f,
        };

        @fragment fn fragmentMain(fragment: FragmentInput) -> @location(0) vec4f {
            var texture_is_srgb: bool = bool(draw_flags & 0x1);
            var target_is_srgb: bool = bool(draw_flags & 0x2);
            var texel = textureSample(draw_texture, draw_sampler, fragment.uv);
            return fragment.color * texel;
        }
    )WGSL";

    ShaderModuleWGSLDescriptor shader_desc;
    shader_desc.code = ui_source;
    ShaderModuleDescriptor module_desc{
        .nextInChain = &shader_desc,
        .label = "ui",
    };
    ShaderModule module = device.CreateShaderModule(&module_desc);
    FutureWaitInfo build_log_future;
    build_log_future.future = module.GetCompilationInfo(CallbackMode::WaitAnyOnly,
        [ui_source] (CompilationInfoRequestStatus status, CompilationInfo const * info) {
            /* if (status == CompilationInfoRequestStatus::Success && info->messageCount > 0) { */
            /*     std::cout << "WGSL Compilation log:\n"; */
            /*     static const char* msg_types[4] = { "", "Error", "Warning", "Info" }; */
            /*     for (int i = 0; i < info->messageCount; i++) { */
            /*         std::string_view excerpt(ui_source + info->messages[i].offset, info->messages[i].length); */
            /*         std::cout */
            /*         << "[" << msg_types[static_cast<uint32_t>(info->messages[i].type)] << "] " */
            /*         << info->messages[i].lineNum << ": " << info->messages[i].message << "\n" */
            /*         << excerpt << "\n"; */
            /*     } */
            /* } */
        }
    );

    WaitStatus status = device.GetAdapter().GetInstance().WaitAny(1, &build_log_future, 0);
    if (status != WaitStatus::Success || !build_log_future.completed) {
        std::cout << "Shader compilation error\n";
        // TODO roll back
    }

    /* DepthStencilState depth_stenci_state{ */
    /*     .format = TextureFormat::DepthStencilState */
    /* Bool depthWriteEnabled = false; */
    /* CompareFunction depthCompare = CompareFunction::Undefined; */
    /* StencilFaceState stencilFront; */
    /* StencilFaceState stencilBack; */
    /* }; */
    BlendState blend_state{
        .color = { .srcFactor = BlendFactor::SrcAlpha, .dstFactor = BlendFactor::OneMinusSrcAlpha },
    };
    ColorTargetState target{
        .format = TextureFormat::BGRA8Unorm,
        .blend = &blend_state,
    };
    FragmentState fragment_state{
        .module =  module,
        .entryPoint = "fragmentMain",
        .targetCount = 1,
        .targets = &target,
    };
    RenderPipelineDescriptor pipeline_desc{
        .label = "ui",
        //.layout = { },
        .vertex = {
            .module = module,
            .entryPoint = "vertexMain",
            .bufferCount = 1,
            .buffers = &vertex_layout,
        },
        //.primitive = ok
        // .depthStencil = nullptr; ok ?
        .fragment = &fragment_state,
    };
    this->pipeline = device.CreateRenderPipeline(&pipeline_desc);

    // view info uniform buffer
    BufferDescriptor view_uniform_desc{
        .usage = BufferUsage::Uniform | BufferUsage::CopyDst,
        .size = 16,
    };
    this->view_uniform_buffer = device.CreateBuffer(&view_uniform_desc);

    // draw flags uniform buffer
    BufferDescriptor draw_flags_uniform_desc{
        .usage = BufferUsage::Uniform | BufferUsage::CopyDst,
        .size = 4,
    };
    this->draw_flags_uniform_buffer = device.CreateBuffer(&draw_flags_uniform_desc);
    uint32_t flags = 0;
    device.GetQueue().WriteBuffer(this->draw_flags_uniform_buffer, 0, &flags, 4);

    BindGroupEntry pass_bindings[3] = {
        { .binding = 0, .buffer = this->view_uniform_buffer },
        { .binding = 1, .buffer = this->draw_flags_uniform_buffer },
        { .binding = 2, .sampler = device.CreateSampler() },
    };
    BindGroupDescriptor bind_group_desc{
        .layout =this->pipeline.GetBindGroupLayout(0),
        .entryCount = 3,
        .entries = pass_bindings,
    };
    this->bind_groups[0] = device.CreateBindGroup(&bind_group_desc);

    // Font glyphs texture atlas
    uint8_t* data;
    int32_t w, h;
    io.Fonts->GetTexDataAsRGBA32(&data, &w, &h);
    uint32_t font_tex_width = static_cast<uint32_t>(w);
    uint32_t font_tex_height = static_cast<uint32_t>(h);
    size_t size = font_tex_width * font_tex_height * 4;

    TextureDescriptor font_tex_desc{
        .label = "ImGui font texture",
        .usage = TextureUsage::TextureBinding | TextureUsage::CopyDst,
        .size = { .width = font_tex_width,
                  .height = font_tex_height },
        .format = TextureFormat::BGRA8Unorm, // XXX oh ?
        //.viewFormatCount = 0;
        // TextureFormat const * viewFormats;
    };

    this->font_texture = device.CreateTexture(&font_tex_desc);
    ImageCopyTexture dest{ .texture = this->font_texture, };
    Extent3D write_size{ .width = font_tex_width, .height = font_tex_height, };
    TextureDataLayout layout{
        .bytesPerRow = font_tex_width * 4,
        /* .rowsPerImage = WGPU_COPY_STRIDE_UNDEFINED; */
    };
    device.GetQueue().WriteTexture(&dest, data, size, &layout, &write_size);
    io.Fonts->SetTexID((void*)&font_texture);

    // font texture bind group
    BindGroupEntry font_texture_entry{
        .binding = 0,
        .textureView = this->font_texture.CreateView(),
    };
    bind_group_desc = {
        .layout = this->pipeline.GetBindGroupLayout(1),
        .entryCount = 1,
        .entries = &font_texture_entry,
    };
    this->bind_groups[1] = device.CreateBindGroup(&bind_group_desc);
    this->last_texture = &this->font_texture;
}

ImGuiWebGPU::~ImGuiWebGPU() {
    if (context) {
        ImGui::DestroyContext(context);
    }
}

void ImGuiWebGPU::begin_frame(uint32_t win_width, uint32_t win_height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(win_width, win_height);

    float view_info[4] = { io.DisplaySize.x, io.DisplaySize.y, 1.0f / io.DisplaySize.x, 1.0f / io.DisplaySize.y };
    this->device.GetQueue().WriteBuffer(this->view_uniform_buffer, 0, view_info, 16);

    ImGui::NewFrame();
}

void ImGuiWebGPU::end_frame(Texture target) {
    ImGui::Render();

    uint32_t draw_flags = is_srgb(target.GetFormat()) << 1 | this->last_draw_flags & 0x1;

    ImDrawData* draw_data = ImGui::GetDrawData();

    CommandEncoder encoder = this->device.CreateCommandEncoder();

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* commands   = draw_data->CmdLists[n];
        const ImDrawVert* ui_vertices = commands->VtxBuffer.Data;
        const ImDrawIdx*  ui_indices = commands->IdxBuffer.Data;

        uint32_t num_vertices = (uint32_t)commands->VtxBuffer.size();

        BufferDescriptor vb_desc{
            .label = "ui vertex buffer",
            .usage = BufferUsage::Vertex | BufferUsage::CopyDst,
            .size = sizeof(ImDrawVert) * num_vertices, // note: ImDrawVert is already a multiple of 4
        };
        Buffer v_buffer = this->device.CreateBuffer(&vb_desc);

        this->device.GetQueue().WriteBuffer(v_buffer, 0, ui_vertices, vb_desc.size);

        uint32_t size = commands->IdxBuffer.size_in_bytes();
        BufferDescriptor ib_desc{
            .label = "ui index buffer",
            .usage = BufferUsage::Index | BufferUsage::CopyDst,
            .size = align_up(size, 4),
        };
        Buffer i_buffer = this->device.CreateBuffer(&ib_desc);

        if (commands->IdxBuffer.capacity() * sizeof(uint16_t) >= ib_desc.size) {
            // use extra capacity to write up to alignment
            this->device.GetQueue().WriteBuffer(i_buffer, 0, ui_indices, ib_desc.size);
        } else {
            uint32_t aligned_size = align_up(size - 4, 4);
            this->device.GetQueue().WriteBuffer(i_buffer, 0, ui_indices, aligned_size);
            uint8_t rest[4];
            memcpy(rest, ((uint8_t *)ui_indices) + aligned_size, size - aligned_size);
            this->device.GetQueue().WriteBuffer(i_buffer, aligned_size, rest, 4);
        }

        // std::cout << "Draw list #" << n << ": " << num_vertices << " vertices, "
        //     << commands->IdxBuffer.size() << " indices\n";

        RenderPassColorAttachment colorAttachment{
            .view = target.CreateView(),
            .loadOp = LoadOp::Load,
            .storeOp = StoreOp::Store,
        };
        RenderPassDescriptor pass_desc{
            .colorAttachmentCount = 1,
            .colorAttachments = &colorAttachment,
        };
        RenderPassEncoder pass = encoder.BeginRenderPass(&pass_desc);

        pass.SetPipeline(this->pipeline);
        pass.SetVertexBuffer(0, v_buffer);
        pass.SetIndexBuffer(i_buffer, IndexFormat::Uint16);
        pass.SetBindGroup(0, this->bind_groups[0]);
        pass.SetBindGroup(1, this->bind_groups[1]);

        for (int cmd = 0; cmd < commands->CmdBuffer.Size; cmd++) {
            const ImDrawCmd* pcmd = &commands->CmdBuffer[cmd];
            if (pcmd->ElemCount == 0) {
                continue;
            }

            if (pcmd->UserCallback) {
                pcmd->UserCallback(commands, pcmd);
            } else {
                pass.SetScissorRect(
                        static_cast<uint32_t>(pcmd->ClipRect.x),
                        static_cast<uint32_t>(pcmd->ClipRect.y),
                        static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                        static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y));

                // TODO: lower to WGPUTexture type and use Acquire/release ?
                Texture* texture = static_cast<Texture*>(pcmd->TextureId);
                if (texture->Get() != this->last_texture->Get()) {
                    if (texture->Get() == this->font_texture.Get()) {
                        pass.SetBindGroup(1, this->bind_groups[1]);
                    } else {
                        BindGroupEntry texture_entry{
                            .binding = 0,
                            .textureView = texture->CreateView(),
                        };
                        BindGroupDescriptor bind_group_desc{
                            .layout = this->pipeline.GetBindGroupLayout(1),
                            .entryCount = 1,
                            .entries = &texture_entry,
                        };
                        this->bind_groups[2] = device.CreateBindGroup(&bind_group_desc);
                        pass.SetBindGroup(1, this->bind_groups[2]);
                    }
                    this->last_texture = texture;
                }

                draw_flags |= is_srgb(texture->GetFormat()) & 0x1;
                if (draw_flags != this->last_draw_flags) {
                   this->device.GetQueue().WriteBuffer(this->draw_flags_uniform_buffer, 0, &draw_flags, 4);
                   this->last_draw_flags = draw_flags;
                }

                pass.DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset, pcmd->VtxOffset);
            }
        }
        pass.End();
    }

    CommandBuffer cmd_buffer = encoder.Finish();
    this->device.GetQueue().Submit(1, &cmd_buffer);
}

ImGuiGlfw::ImGuiGlfw(GLFWwindow * window) : window(window) {
    register_callbacks(window);
}

ImGuiGlfw::~ImGuiGlfw() {
    unregister_callbacks(this->window);
}

bool ImGuiGlfw::is_idle() {
    this->refresh_frame_count = this->refresh_frame_count > 0 ? this->refresh_frame_count - 1 : 0;
    return this->refresh_frame_count == 0;
}

void ImGuiGlfw::register_callbacks(GLFWwindow * window) {
    std::cout << "Registering ImGUI\n";
    this->prev_window_pointer = glfwGetWindowUserPointer(window);
    this->prev_window_size_cb = glfwSetWindowSizeCallback(window, ImGuiGlfw::window_size_cb);
    this->prev_mouse_button_cb = glfwSetMouseButtonCallback(window, ImGuiGlfw::mouse_button_cb);
    this->prev_cursor_pos_cb = glfwSetCursorPosCallback(window, ImGuiGlfw::cursor_pos_cb);
    this->prev_cursor_enter_cb = glfwSetCursorEnterCallback(window, ImGuiGlfw::cursor_enter_cb);
    this->prev_scroll_cb = glfwSetScrollCallback(window, ImGuiGlfw::scroll_cb);
    this->prev_key_cb = glfwSetKeyCallback(window, ImGuiGlfw::key_cb);
    this->prev_char_cb = glfwSetCharCallback(window, ImGuiGlfw::char_cb);
    this->prev_char_mods_cb = glfwSetCharModsCallback(window, ImGuiGlfw::char_mods_cb);

    glfwSetWindowUserPointer(window, this);
}

void ImGuiGlfw::unregister_callbacks(GLFWwindow * window) {
    std::cout << "Unregistering ImGUI\n";
    glfwSetWindowUserPointer(window, this->prev_window_pointer);
    glfwSetWindowSizeCallback(window, this->prev_window_size_cb);
    glfwSetMouseButtonCallback(window, this->prev_mouse_button_cb);
    glfwSetCursorPosCallback(window, this->prev_cursor_pos_cb);
    glfwSetCursorEnterCallback(window, this->prev_cursor_enter_cb);
    glfwSetScrollCallback(window, this->prev_scroll_cb);
    glfwSetKeyCallback(window, this->prev_key_cb);
    glfwSetCharCallback(window, this->prev_char_cb);
    //glfwSetCharModsCallback(window, this->prev_char_mods_cb);
}

static void set_modifiers(GLFWwindow *window) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl,  (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)   == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Alt,   (glfwGetKey(window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_ALT)     == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(window, GLFW_KEY_LEFT_SUPER)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SUPER)   == GLFW_PRESS));
}

// from ImGUI distro sample GLTF backend
static int translate_untranslated_key(int key, int scancode)
{
#if GLFW_HAS_GETKEYNAME && !defined(__EMSCRIPTEN__)
    // GLFW 3.1+ attempts to "untranslate" keys, which goes the opposite of what every other framework does, making using lettered shortcuts difficult.
    // (It had reasons to do so: namely GLFW is/was more likely to be used for WASD-type game controls rather than lettered shortcuts, but IHMO the 3.1 change could have been done differently)
    // See https://github.com/glfw/glfw/issues/1502 for details.
    // Adding a workaround to undo this (so our keys are translated->untranslated->translated, likely a lossy process).
    // This won't cover edge cases but this is at least going to cover common cases.
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_EQUAL)
        return key;
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
    const char* key_name = glfwGetKeyName(key, scancode);
    glfwSetErrorCallback(prev_error_callback);
#if GLFW_HAS_GETERROR && !defined(__EMSCRIPTEN__) // Eat errors (see #5908)
    (void)glfwGetError(nullptr);
#endif
    if (key_name && key_name[0] != 0 && key_name[1] == 0)
    {
        const char char_names[] = "`-=[]\\,;\'./";
        const int char_keys[] = { GLFW_KEY_GRAVE_ACCENT, GLFW_KEY_MINUS, GLFW_KEY_EQUAL, GLFW_KEY_LEFT_BRACKET, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_COMMA, GLFW_KEY_SEMICOLON, GLFW_KEY_APOSTROPHE, GLFW_KEY_PERIOD, GLFW_KEY_SLASH, 0 };
        IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
        if (key_name[0] >= '0' && key_name[0] <= '9')               { key = GLFW_KEY_0 + (key_name[0] - '0'); }
        else if (key_name[0] >= 'A' && key_name[0] <= 'Z')          { key = GLFW_KEY_A + (key_name[0] - 'A'); }
        else if (key_name[0] >= 'a' && key_name[0] <= 'z')          { key = GLFW_KEY_A + (key_name[0] - 'a'); }
        else if (const char* p = strchr(char_names, key_name[0]))   { key = char_keys[p - char_names]; }
    }
    // if (action == GLFW_PRESS) printf("key %d scancode %d name '%s'\n", key, scancode, key_name);
#else
    IM_UNUSED(scancode);
#endif
    return key;
}

// from ImGUI distro sample GLTF backend
static ImGuiKey to_imgui_key(int key)
{
    switch (key)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        case GLFW_KEY_F13: return ImGuiKey_F13;
        case GLFW_KEY_F14: return ImGuiKey_F14;
        case GLFW_KEY_F15: return ImGuiKey_F15;
        case GLFW_KEY_F16: return ImGuiKey_F16;
        case GLFW_KEY_F17: return ImGuiKey_F17;
        case GLFW_KEY_F18: return ImGuiKey_F18;
        case GLFW_KEY_F19: return ImGuiKey_F19;
        case GLFW_KEY_F20: return ImGuiKey_F20;
        case GLFW_KEY_F21: return ImGuiKey_F21;
        case GLFW_KEY_F22: return ImGuiKey_F22;
        case GLFW_KEY_F23: return ImGuiKey_F23;
        case GLFW_KEY_F24: return ImGuiKey_F24;
        default: return ImGuiKey_None;
    }
}


void ImGuiGlfw::window_size_cb(GLFWwindow* window, int width, int height) {
    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_window_size_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_window_size_cb(window, width, height);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::mouse_button_cb(GLFWwindow *window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();

    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);

    set_modifiers(window);

    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 3;
    if (self->prev_mouse_button_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_mouse_button_cb(window, button, action, mods);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::cursor_pos_cb(GLFWwindow *window, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)xpos, (float)ypos);

    // TODO
    // this->LastValidMousePos = ImVec2((float)x, (float)y);

    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_cursor_pos_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_cursor_pos_cb(window, xpos, ypos);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::cursor_enter_cb(GLFWwindow *window, int entered) {
    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_cursor_enter_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_cursor_enter_cb(window, entered);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::scroll_cb(GLFWwindow *window, double xoffset, double yoffset) {
    // TODO ignore under Emscripten

    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);

    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_scroll_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_scroll_cb(window, xoffset, yoffset);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::key_cb(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_RELEASE) {
        return;
    }

    set_modifiers(window);
    key = translate_untranslated_key(key, scancode);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiKey imgui_key = to_imgui_key(key);
    io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
    io.SetKeyEventNativeData(imgui_key, key, scancode); // To support legacy indexing (<1.87 user code)

    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_key_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_key_cb(window, key, scancode, action, mods);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::char_cb(GLFWwindow *window, unsigned int codepoint) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(codepoint);

    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_char_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_char_cb(window, codepoint);
        glfwSetWindowUserPointer(window, self);
    }
}

void ImGuiGlfw::char_mods_cb(GLFWwindow *window, unsigned int codepoint, int mods) {
    auto * self = reinterpret_cast<ImGuiGlfw*>(glfwGetWindowUserPointer(window));
    self->refresh_frame_count = 2;
    if (self->prev_char_mods_cb) {
        glfwSetWindowUserPointer(window, self->prev_window_pointer);
        self->prev_char_mods_cb(window, codepoint, mods);
        glfwSetWindowUserPointer(window, self);
    }
}
