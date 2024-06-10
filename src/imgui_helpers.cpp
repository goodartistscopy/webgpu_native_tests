#include "imgui_helpers.h"

/* #include <algorithm> */
/* #include <unordered_map> */
#include <iostream>
#include <string_view>

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

    /* io.DisplaySize = ImVec2(winWidth, winHeight); */
    /* io.IniFilename = nullptr; */

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

/* ImGuiHelper::~ImGuiHelper() */
/* { */
/*     if (m_context) */
/*     { */
/*         ImGui::DestroyContext(m_context); */
/*     } */

/*     TextureManager::getInstance().remove(s_fontTextureName); */

/*     for (const auto& mesh : m_meshes) */
/*     { */
/*         MeshManager::getInstance().remove(mesh); */
/*     } */

/*     if (m_scene) */
/*     { */
/*         for (const auto& entity : m_entities) */
/*         { */
/*             m_scene->removeEntity(entity); */
/*         } */
/*     } */
/* } */

/* void ImGuiHelper::init(size_t winWidth, size_t winHeight, Chameleon::ScenePtr scene) */
/* { */
/*     m_context = ImGui::CreateContext(); */
/*     m_scene   = scene; */

/*     m_meshes.clear(); */
/*     m_entities.clear(); */

/*     ImGuiIO& io = ImGui::GetIO(); */

/*     io.DisplaySize = ImVec2(winWidth, winHeight); */
/*     io.IniFilename = nullptr; */

/*     ImGuiStyle& style = ImGui::GetStyle(); */
/*     ImGui::StyleColorsClassic(&style); */
/*     style.FrameRounding    = 4.0f; */
/*     style.WindowBorderSize = 0.0f; */
/*     style.WindowPadding    = ImVec2(10.0f, 10.0f); */

/*     style.AntiAliasedLines       = true; */
/*     style.AntiAliasedLinesUseTex = false; */

/*     uint8_t* data; */
/*     int32_t  texWidth; */
/*     int32_t  texHeight; */
/*     io.Fonts->GetTexDataAsRGBA32(&data, &texWidth, &texHeight); */
/*     io.Fonts->SetTexID((void*)&s_fontTextureName); */
/*     size_t size = texWidth * texHeight * 4; */
/*     // assume the ImGui data outlive the texture */
/*     TextureUPtr tex = std::make_unique<Texture>(Texture::makeLayout(Texture::Format::BGRA8, texWidth, texHeight), MemBuffer(data, size)); */
/*     TextureManager::getInstance().add(s_fontTextureName, std::move(tex)); */
/* } */

/* void ImGuiHelper::resize(size_t winWidth, size_t winHeight) */
/* { */
/*     ImGuiIO& io    = ImGui::GetIO(); */
/*     io.DisplaySize = ImVec2(winWidth, winHeight); */
/* } */

/* ImGuiContext* ImGuiHelper::getContext() const */
/* { */
/*     return m_context; */
/* } */

/* void ImGuiHelper::setModifiers(int mods) const */
/* { */
/*     ImGuiIO& io = ImGui::GetIO(); */
/*     io.AddKeyEvent(ImGuiKey_ModCtrl, (mods & UI::Modifier::Enum::Ctrl) != 0); */
/*     io.AddKeyEvent(ImGuiKey_ModShift, (mods & UI::Modifier::Enum::Shift) != 0); */
/*     io.AddKeyEvent(ImGuiKey_ModAlt, (mods & UI::Modifier::Enum::Alt) != 0); */
/*     io.AddKeyEvent(ImGuiKey_ModSuper, (mods & UI::Modifier::Enum::Super) != 0); */
/* } */

/* void ImGuiHelper::setKeyState(UI::Key::Enum key, UI::Action::Enum action, int mods) const */
/* { */
/*     if (action != UI::Action::Enum::Press && action != UI::Action::Release) */
/*     { */
/*         return; */
/*     } */

/*     setModifiers(mods); */

/*     ImGuiIO& io       = ImGui::GetIO(); */
/*     ImGuiKey imGuiKey = convertToImGuiKey(key); */
/*     io.AddKeyEvent(imGuiKey, (action == UI::Action::Enum::Press)); */
///*     io.SetKeyEventNativeData(imGuiKey, static_cast<int>(key), 0 /*scancode*/); // To support legacy indexing (<1.87 user code) */
/* } */

/* void ImGuiHelper::setCharacter(unsigned int c) const */
/* { */
/*     ImGuiIO& io = ImGui::GetIO(); */
/*     io.AddInputCharacter(c); */
/* } */

/* void ImGuiHelper::setMousePosition(float x, float y) */
/* { */
/*     ImGuiIO& io = ImGui::GetIO(); */
/*     io.MousePos = ImVec2(x, y); */
/* } */

/* void ImGuiHelper::setButtonState(UI::Mouse::Enum button, UI::Action::Enum action, int mods) */
/* { */
/*     ImGuiIO& io = ImGui::GetIO(); */

/*     io.MouseDown[0] = (button == UI::Mouse::Enum::Left) & (action == UI::Action::Enum::Press); */
/*     io.MouseDown[1] = (button == UI::Mouse::Enum::Right) & (action == UI::Action::Enum::Press); */
/*     io.MouseDown[2] = (button == UI::Mouse::Enum::Middle) & (action == UI::Action::Enum::Press); */

/*     setModifiers(mods); */
/* } */

/* void ImGuiHelper::setWheelScroll(double amount) */
/* { */
/*     ImGuiIO& io = ImGui::GetIO(); */

/*     io.MouseWheel = amount; */
/* } */

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
            .size = sizeof(ImDrawVert) * num_vertices,
        };
        Buffer v_buffer = this->device.CreateBuffer(&vb_desc);

        this->device.GetQueue().WriteBuffer(v_buffer, 0, ui_vertices, vb_desc.size);

        // FIXME : enforcing size to be a multiple of 4...
        uint32_t size = ((static_cast<uint32_t>(commands->IdxBuffer.size()) + 1) >> 1) << 1;
        std::cout << "nElems " << commands->IdxBuffer.size() << " -> " << size << "\n";
        BufferDescriptor ib_desc{
            .label = "ui index buffer",
            .usage = BufferUsage::Index | BufferUsage::CopyDst,
            .size = sizeof(uint16_t) * size, //(uint32_t)commands->IdxBuffer.size(),
        };
        Buffer i_buffer = this->device.CreateBuffer(&ib_desc);

        this->device.GetQueue().WriteBuffer(i_buffer, 0, ui_indices, ib_desc.size);
        
        std::cout << "Draw list #" << n << ": " << num_vertices << " vertices, "
            << commands->IdxBuffer.size() << " indices\n";

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
                // - To use 16-bit indices + allow large meshes: backend need to set 'io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset' and handle ImDrawCmd::VtxOffset (recommended).

                pass.SetScissorRect(
                        static_cast<uint32_t>(pcmd->ClipRect.x),
                        static_cast<uint32_t>(pcmd->ClipRect.y),
                        static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x),
                        static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y));

                // TODO: lowerto WGPUTexture type and use Acquire/release ?
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

                pass.DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset /*, pcmd->VtxOffset */);
            }
        }
        pass.End();
    }

    CommandBuffer cmd_buffer = encoder.Finish();
    this->device.GetQueue().Submit(1, &cmd_buffer);
}

/* void ImGuiHelper::end() */
/* { */
/*     if (!m_scene) */
/*     { */
/*         LOG(error) << "ImGuiHelper not initialized"; */
/*         return; */
/*     } */

/*     // first remove all entities from the previous frame */
/*     for (const auto& entityName : m_entities) */
/*     { */
/*         m_scene->removeEntity(entityName); */
/*     } */
/*     m_entities.clear(); */

/*     m_meshes.clear(); */

/*     ImGui::Render(); */
/*     ImDrawData* drawData = ImGui::GetDrawData(); */

/*     LOG(debug) << "IMGUI: command list count: " << drawData->CmdListsCount; */
/*     uint16_t seqId = 1; */
/*     for (int n = 0; n < drawData->CmdListsCount; n++) */
/*     { */
/*         const ImDrawList* cmdList   = drawData->CmdLists[n]; */
/*         const ImDrawVert* vtxBuffer = cmdList->VtxBuffer.Data; */
/*         const ImDrawIdx*  idxBuffer = cmdList->IdxBuffer.Data; */

/*         uint32_t numVertices = (uint32_t)cmdList->VtxBuffer.size(); */

/*         VertexBuffer* vertices = new VertexBuffer(); */
/*         vertices->resize(numVertices); */
/*         vertices->addAttribute(Attribute::Position, AttributeType::Float, 3); */
/*         vertices->addAttribute(Attribute::TexCoord0, AttributeType::Float, 2); */
/*         vertices->addAttribute(Attribute::Color0, AttributeType::Float, 4); */

/*         AttributeArray<float> pos  = vertices->get<float>(Attribute::Position); */
/*         AttributeArray<float> tcs  = vertices->get<float>(Attribute::TexCoord0); */
/*         AttributeArray<float> cols = vertices->get<float>(Attribute::Color0); */

/*         using vec4 = std::array<double, 4>; */
/*         using vec2 = std::array<double, 2>; */
/*         using vec3 = std::array<double, 3>; */

/*         for (uint16_t i = 0; i < numVertices; i++) */
/*         { */
/*             pos[i]       = vec3 {vtxBuffer[i].pos.x, vtxBuffer[i].pos.y, 0}; */
/*             tcs[i]       = vec2 {vtxBuffer[i].uv.x, vtxBuffer[i].uv.y}; */
/*             ImVec4 color = ImGui::ColorConvertU32ToFloat4(vtxBuffer[i].col); */
/*             cols[i]      = vec4 {color.x, color.y, color.z, color.w}; */
/*         } */
/*         MeshUPtr imGuiMesh = std::make_unique<Mesh>(); */
/*         imGuiMesh->addVertexBuffer(std::unique_ptr<VertexBuffer>(vertices)); */

/*         LOG(debug) << "IMGUI: command buffer count: " << cmdList->CmdBuffer.Size; */

/*         struct SubMeshParams */
/*         { */
/*             glm::vec4    scissor; */
/*             std::string* textureId; */
/*         }; */

/*         std::unordered_map<std::string, SubMeshParams> extraParams; */
/*         for (int cmd = 0; cmd < cmdList->CmdBuffer.Size; cmd++) */
/*         { */
/*             const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd]; */
/*             if (pcmd->ElemCount == 0) */
/*             { */
/*                 continue; */
/*             } */

/*             if (pcmd->UserCallback) */
/*             { */
/*                 pcmd->UserCallback(cmdList, pcmd); */
/*             } */
/*             else */
/*             { */
/*                 IndexBuffer<uint16_t>* faces = new IndexBuffer<uint16_t>(PrimitiveType::Triangles); */
/*                 faces->resize(pcmd->ElemCount / 3); */
/*                 using face = std::array<uint16_t, 3>; */

/*                 uint32_t offset = pcmd->IdxOffset; */
/*                 for (uint16_t i = 0; i < pcmd->ElemCount / 3; i++) */
/*                 { */
/*                     (*faces)[i] = face {idxBuffer[offset + 0], idxBuffer[offset + 2], idxBuffer[offset + 1]}; */
/*                     float depth = 1.0 / seqId; */

/*                     pos[idxBuffer[offset + 0]][2] = depth; */
/*                     pos[idxBuffer[offset + 1]][2] = depth; */
/*                     pos[idxBuffer[offset + 2]][2] = depth; */
/*                     offset += 3; */
/*                 } */
/*                 seqId++; */

/*                 std::string subMeshName = std::to_string(cmd); */

/*                 LOG(debug) << "IMGUI: adding subMesh: " << subMeshName; */
/*                 imGuiMesh->addSubMesh(subMeshName, std::unique_ptr<IndexBufferBase>(faces)); */

/*                 extraParams[subMeshName].textureId = (std::string*)pcmd->TextureId; */
/*                 extraParams[subMeshName].scissor = */
/*                     glm::vec4(pcmd->ClipRect.x, pcmd->ClipRect.y, pcmd->ClipRect.z - pcmd->ClipRect.x, pcmd->ClipRect.w - pcmd->ClipRect.y); */
/*             } */
/*         } */

/*         std::string meshName = s_baseMeshName + std::to_string(n); */
/*         MeshManager::getInstance().replace(meshName, std::move(imGuiMesh)); */
/*         LOG(debug) << "IMGUI: adding mesh: " << meshName; */

/*         m_meshes.push_back(meshName); */

/*         MeshPtr mesh = MeshManager::getInstance().get(meshName).lock(); */
/*         for (const auto& subMeshName : mesh->getSubMeshNames()) */
/*         { */
/*             std::string entityName = meshName + "." + subMeshName; */
/*             LOG(debug) << "IMGUI: adding entity: " << entityName; */
/*             Entity* entity = m_scene->addEntity(entityName, meshName, subMeshName); */
/*             entity->set("imgui"); */
/*             entity->setAttribute("scissor", extraParams[subMeshName].scissor); */
/*             if (extraParams[subMeshName].textureId) */
/*             { */
/*                 std::string& texId = *extraParams[subMeshName].textureId; */

/*                 LOG(debug) << "IMGUI: Texture id: " << texId; */
/*                 entity->setAttribute("texColor", Sampler(BaseType::Sampler2D, texId)); */

/*                 TexturePtr tex = TextureManager::getInstance().get(texId).lock(); */
/*                 if (tex && tex->getLayout().m_srgb) */
/*                 { */
/*                     // it's only used as a flag, value is not important */
/*                     entity->setAttribute("texIssRGB", glm::vec4(0.0)); */
/*                 } */

/*                 if (extraParams[subMeshName].textureId != &s_fontTextureName) */
/*                 { */
/*                     delete extraParams[subMeshName].textureId; */
/*                 } */
/*             } */

/*             m_entities.push_back(entityName); */
/*         } */
/*     } */
/* } */

/* ImTextureID ImGuiHelper::createTextureId(const std::string& id) */
/* { */
/*     return new std::string(id); */
/* } */

/* ImGuiKey convertToImGuiKey(UI::Key::Enum key) */
/* { */
/*     switch (key) */
/*     { */
/*         case UI::Key::Enum::Tab: */
/*             return ImGuiKey_Tab; */
/*         case UI::Key::Enum::LeftArrow: */
/*             return ImGuiKey_LeftArrow; */
/*         case UI::Key::Enum::RightArrow: */
/*             return ImGuiKey_RightArrow; */
/*         case UI::Key::Enum::UpArrow: */
/*             return ImGuiKey_UpArrow; */
/*         case UI::Key::Enum::DownArrow: */
/*             return ImGuiKey_DownArrow; */
/*         case UI::Key::Enum::PageUp: */
/*             return ImGuiKey_PageUp; */
/*         case UI::Key::Enum::PageDown: */
/*             return ImGuiKey_PageDown; */
/*         case UI::Key::Enum::Home: */
/*             return ImGuiKey_Home; */
/*         case UI::Key::Enum::End: */
/*             return ImGuiKey_End; */
/*         case UI::Key::Enum::Insert: */
/*             return ImGuiKey_Insert; */
/*         case UI::Key::Enum::Delete: */
/*             return ImGuiKey_Delete; */
/*         case UI::Key::Enum::Backspace: */
/*             return ImGuiKey_Backspace; */
/*         case UI::Key::Enum::Space: */
/*             return ImGuiKey_Space; */
/*         case UI::Key::Enum::Enter: */
/*             return ImGuiKey_Enter; */
/*         case UI::Key::Enum::Escape: */
/*             return ImGuiKey_Escape; */
/*         case UI::Key::Enum::Apostrophe: */
/*             return ImGuiKey_Apostrophe; */
/*         case UI::Key::Enum::Comma: */
/*             return ImGuiKey_Comma; */
/*         case UI::Key::Enum::Minus: */
/*             return ImGuiKey_Minus; */
/*         case UI::Key::Enum::Period: */
/*             return ImGuiKey_Period; */
/*         case UI::Key::Enum::Slash: */
/*             return ImGuiKey_Slash; */
/*         case UI::Key::Enum::Semicolon: */
/*             return ImGuiKey_Semicolon; */
/*         case UI::Key::Enum::Equal: */
/*             return ImGuiKey_Equal; */
/*         case UI::Key::Enum::LeftBracket: */
/*             return ImGuiKey_LeftBracket; */
/*         case UI::Key::Enum::Backslash: */
/*             return ImGuiKey_Backslash; */
/*         case UI::Key::Enum::RightBracket: */
/*             return ImGuiKey_RightBracket; */
/*         case UI::Key::Enum::GraveAccent: */
/*             return ImGuiKey_GraveAccent; */
/*         case UI::Key::Enum::CapsLock: */
/*             return ImGuiKey_CapsLock; */
/*         case UI::Key::Enum::ScrollLock: */
/*             return ImGuiKey_ScrollLock; */
/*         case UI::Key::Enum::NumLock: */
/*             return ImGuiKey_NumLock; */
/*         case UI::Key::Enum::PrintScreen: */
/*             return ImGuiKey_PrintScreen; */
/*         case UI::Key::Enum::Pause: */
/*             return ImGuiKey_Pause; */
/*         case UI::Key::Enum::Keypad0: */
/*             return ImGuiKey_Keypad0; */
/*         case UI::Key::Enum::Keypad1: */
/*             return ImGuiKey_Keypad1; */
/*         case UI::Key::Enum::Keypad2: */
/*             return ImGuiKey_Keypad2; */
/*         case UI::Key::Enum::Keypad3: */
/*             return ImGuiKey_Keypad3; */
/*         case UI::Key::Enum::Keypad4: */
/*             return ImGuiKey_Keypad4; */
/*         case UI::Key::Enum::Keypad5: */
/*             return ImGuiKey_Keypad5; */
/*         case UI::Key::Enum::Keypad6: */
/*             return ImGuiKey_Keypad6; */
/*         case UI::Key::Enum::Keypad7: */
/*             return ImGuiKey_Keypad7; */
/*         case UI::Key::Enum::Keypad8: */
/*             return ImGuiKey_Keypad8; */
/*         case UI::Key::Enum::Keypad9: */
/*             return ImGuiKey_Keypad9; */
/*         case UI::Key::Enum::KeypadDecimal: */
/*             return ImGuiKey_KeypadDecimal; */
/*         case UI::Key::Enum::KeypadDivide: */
/*             return ImGuiKey_KeypadDivide; */
/*         case UI::Key::Enum::KeypadMultiply: */
/*             return ImGuiKey_KeypadMultiply; */
/*         case UI::Key::Enum::KeypadSubtract: */
/*             return ImGuiKey_KeypadSubtract; */
/*         case UI::Key::Enum::KeypadAdd: */
/*             return ImGuiKey_KeypadAdd; */
/*         case UI::Key::Enum::KeypadEnter: */
/*             return ImGuiKey_KeypadEnter; */
/*         case UI::Key::Enum::KeypadEqual: */
/*             return ImGuiKey_KeypadEqual; */
/*         case UI::Key::Enum::LeftShift: */
/*             return ImGuiKey_LeftShift; */
/*         case UI::Key::Enum::LeftCtrl: */
/*             return ImGuiKey_LeftCtrl; */
/*         case UI::Key::Enum::LeftAlt: */
/*             return ImGuiKey_LeftAlt; */
/*         case UI::Key::Enum::LeftSuper: */
/*             return ImGuiKey_LeftSuper; */
/*         case UI::Key::Enum::RightShift: */
/*             return ImGuiKey_RightShift; */
/*         case UI::Key::Enum::RightCtrl: */
/*             return ImGuiKey_RightCtrl; */
/*         case UI::Key::Enum::RightAlt: */
/*             return ImGuiKey_RightAlt; */
/*         case UI::Key::Enum::RightSuper: */
/*             return ImGuiKey_RightSuper; */
/*         case UI::Key::Enum::Menu: */
/*             return ImGuiKey_Menu; */
/*         case UI::Key::Enum::Key0: */
/*             return ImGuiKey_0; */
/*         case UI::Key::Enum::Key1: */
/*             return ImGuiKey_1; */
/*         case UI::Key::Enum::Key2: */
/*             return ImGuiKey_2; */
/*         case UI::Key::Enum::Key3: */
/*             return ImGuiKey_3; */
/*         case UI::Key::Enum::Key4: */
/*             return ImGuiKey_4; */
/*         case UI::Key::Enum::Key5: */
/*             return ImGuiKey_5; */
/*         case UI::Key::Enum::Key6: */
/*             return ImGuiKey_6; */
/*         case UI::Key::Enum::Key7: */
/*             return ImGuiKey_7; */
/*         case UI::Key::Enum::Key8: */
/*             return ImGuiKey_8; */
/*         case UI::Key::Enum::Key9: */
/*             return ImGuiKey_9; */
/*         case UI::Key::Enum::KeyA: */
/*             return ImGuiKey_A; */
/*         case UI::Key::Enum::KeyB: */
/*             return ImGuiKey_B; */
/*         case UI::Key::Enum::KeyC: */
/*             return ImGuiKey_C; */
/*         case UI::Key::Enum::KeyD: */
/*             return ImGuiKey_D; */
/*         case UI::Key::Enum::KeyE: */
/*             return ImGuiKey_E; */
/*         case UI::Key::Enum::KeyF: */
/*             return ImGuiKey_F; */
/*         case UI::Key::Enum::KeyG: */
/*             return ImGuiKey_G; */
/*         case UI::Key::Enum::KeyH: */
/*             return ImGuiKey_H; */
/*         case UI::Key::Enum::KeyI: */
/*             return ImGuiKey_I; */
/*         case UI::Key::Enum::KeyJ: */
/*             return ImGuiKey_J; */
/*         case UI::Key::Enum::KeyK: */
/*             return ImGuiKey_K; */
/*         case UI::Key::Enum::KeyL: */
/*             return ImGuiKey_L; */
/*         case UI::Key::Enum::KeyM: */
/*             return ImGuiKey_M; */
/*         case UI::Key::Enum::KeyN: */
/*             return ImGuiKey_N; */
/*         case UI::Key::Enum::KeyO: */
/*             return ImGuiKey_O; */
/*         case UI::Key::Enum::KeyP: */
/*             return ImGuiKey_P; */
/*         case UI::Key::Enum::KeyQ: */
/*             return ImGuiKey_Q; */
/*         case UI::Key::Enum::KeyR: */
/*             return ImGuiKey_R; */
/*         case UI::Key::Enum::KeyS: */
/*             return ImGuiKey_S; */
/*         case UI::Key::Enum::KeyT: */
/*             return ImGuiKey_T; */
/*         case UI::Key::Enum::KeyU: */
/*             return ImGuiKey_U; */
/*         case UI::Key::Enum::KeyV: */
/*             return ImGuiKey_V; */
/*         case UI::Key::Enum::KeyW: */
/*             return ImGuiKey_W; */
/*         case UI::Key::Enum::KeyX: */
/*             return ImGuiKey_X; */
/*         case UI::Key::Enum::KeyY: */
/*             return ImGuiKey_Y; */
/*         case UI::Key::Enum::KeyZ: */
/*             return ImGuiKey_Z; */
/*         case UI::Key::Enum::F1: */
/*             return ImGuiKey_F1; */
/*         case UI::Key::Enum::F2: */
/*             return ImGuiKey_F2; */
/*         case UI::Key::Enum::F3: */
/*             return ImGuiKey_F3; */
/*         case UI::Key::Enum::F4: */
/*             return ImGuiKey_F4; */
/*         case UI::Key::Enum::F5: */
/*             return ImGuiKey_F5; */
/*         case UI::Key::Enum::F6: */
/*             return ImGuiKey_F6; */
/*         case UI::Key::Enum::F7: */
/*             return ImGuiKey_F7; */
/*         case UI::Key::Enum::F8: */
/*             return ImGuiKey_F8; */
/*         case UI::Key::Enum::F9: */
/*             return ImGuiKey_F9; */
/*         case UI::Key::Enum::F10: */
/*             return ImGuiKey_F10; */
/*         case UI::Key::Enum::F11: */
/*             return ImGuiKey_F11; */
/*         case UI::Key::Enum::F12: */
/*             return ImGuiKey_F12; */
/*         default: */
/*             return ImGuiKey_None; */
/*     } */
/* } */
