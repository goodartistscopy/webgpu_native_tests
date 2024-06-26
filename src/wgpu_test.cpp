#include <_types/_uint32_t.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cassert>
#include <chrono>
#include <string>
#include <unistd.h>
#include <vector>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>

#include "imgui_helpers.h"
#include "webgpu_helpers.h"

using namespace wgpu;

struct GPU {
    Instance instance; // XXX could avoid and use adapter.GetInstance()
    Adapter adapter; // XXX could avoid and use device.GetAdapter()
    Device device;
    std::unordered_map<GLFWwindow*, Surface> surfaces;

    void release() {
        std::cout << "Cleaning up...\n";

        for (auto& windows : surfaces) {
            windows.second.Unconfigure();
            glfwDestroyWindow(windows.first);
        }
        device.Destroy();
    }
};

[[nodiscard]] GPU init_webgpu() {
    GPU gpu{};
    gpu.instance = CreateInstance();

    RequestAdapterOptions options = {
        .compatibleSurface = nullptr,
        .powerPreference = PowerPreference::HighPerformance,
        .forceFallbackAdapter = false,
        .compatibilityMode = false,
    };
    FutureWaitInfo adapter_future;
    adapter_future.future = gpu.instance.RequestAdapter(&options, CallbackMode::WaitAnyOnly,
        [&gpu] (RequestAdapterStatus status, Adapter adapter, char const* message) {
            assert(status == RequestAdapterStatus::Success);
            gpu.adapter = adapter;
    });

    auto status = gpu.instance.WaitAny(1, &adapter_future, 0);
    if (status == WaitStatus::Success && adapter_future.completed) {
        DeviceDescriptor options = {
            .deviceLostCallbackInfo = {
                .callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, char const * message, void *) {
                    std::cout << "Device Lost: " << message << "\n";
                },
            },
            .uncapturedErrorCallbackInfo = {
                .callback = [](WGPUErrorType type, char const * message, void *) {
                    std::cout << "Error: " << message << "\n";
                },
            },
        };
        FutureWaitInfo device_future;
        device_future.future = gpu.adapter.RequestDevice(&options, CallbackMode::WaitAnyOnly,
            [&gpu](RequestDeviceStatus status, Device device, char const* msg) {
                assert(status == RequestDeviceStatus::Success);
                gpu.device = device;
            });
        auto status = gpu.instance.WaitAny(1, &device_future, 0);
        if (status == WaitStatus::Success && device_future.completed) {
        }
    }

    return gpu;
}

void on_window_resized(GLFWwindow* window, int width, int height) {
    GPU* gpu = reinterpret_cast<GPU*>(glfwGetWindowUserPointer(window));
    if (auto surface = gpu->surfaces.find(window); surface != gpu->surfaces.end()) {
        SurfaceConfiguration config{
            .device = gpu->device,
            .format = TextureFormat::BGRA8Unorm,
            .usage = TextureUsage::RenderAttachment | TextureUsage::CopySrc,
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .presentMode = PresentMode::Fifo,
        };
        surface->second.Configure(&config);
    } else {
        std::cout << "Resize: surface not found\n";
    }
}

GLFWwindow* create_window(GPU &gpu, int width, int height) {
    GLFWwindow* window = glfwCreateWindow(width, height, "WebGPU", NULL, NULL);
    if (!window) {
        return nullptr;
    }

    glfwSetWindowUserPointer(window, &gpu);
    glfwSetWindowSizeCallback(window, on_window_resized);

    Surface surface = glfw::CreateSurfaceForWindow(gpu.instance, window);
    if (!surface) {
        std::cout << "Could not initialize surface for window\n";
        glfwDestroyWindow(window);
        return nullptr;
    }

    SurfaceConfiguration config{
        .device = gpu.device,
        .format = TextureFormat::BGRA8Unorm,
        .usage = TextureUsage::RenderAttachment | TextureUsage::CopySrc,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .presentMode = PresentMode::Fifo,
    };
    surface.Configure(&config);

    gpu.surfaces[window] = surface;

    SurfaceCapabilities capabilities;
    if (surface.GetCapabilities(gpu.adapter, &capabilities)) {
        TextureFormat preferred_format = capabilities.formats[0];
        std::cout << "Supported surface formats: [";
        for (int i = 0; i < capabilities.formatCount; i++) {
            auto format = capabilities.formats[i];
            std::cout << format << ((format == preferred_format) ? "*" : "")
                << ((i + 1 < capabilities.formatCount) ? ", " : "");
        }
        std::cout << "]\n";
    }

    return window;
}

static const char shader_code[] = R"WGSL(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) ->
      @builtin(position) vec4f {
        const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
        return vec4f(pos[i], 0, 1);
    }

    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1, 0, 0, 1);
    }
)WGSL";

bool get_window_surface(const GPU &gpu, GLFWwindow *window, Surface &surface, SurfaceTexture &texture) {
    auto it = gpu.surfaces.find(window);
    if (it == gpu.surfaces.end()) {
        surface = Surface();
        texture = SurfaceTexture();
        return false;
    }

    surface = it->second;
    surface.GetCurrentTexture(&texture);
    if (texture.status != SurfaceGetCurrentTextureStatus::Success) {
        std::cout << "surface texture not available\n";
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto gpu = init_webgpu();

    ImGuiWebGPU imgui(gpu.device);

    GLFWwindow* window = create_window(gpu, 1024, 768);
    if (!window) {
        gpu.release();
        glfwTerminate();
        return -1;
    }

    ShaderModuleWGSLDescriptor wgsl_desc;
    wgsl_desc.code = shader_code;

    ShaderModuleDescriptor shader_desc{
        .nextInChain = &wgsl_desc,
    };

    ShaderModule shader_module = gpu.device.CreateShaderModule(&shader_desc);

    ColorTargetState color_target_state{
        .format = TextureFormat::BGRA8Unorm
    };

    FragmentState fragment_state{
        .module = shader_module,
        .targetCount = 1,
        .targets = &color_target_state
    };

    RenderPipelineDescriptor descriptor{
        .vertex = { .module = shader_module },
        .fragment = &fragment_state
    };
    RenderPipeline pipeline = gpu.device.CreateRenderPipeline(&descriptor);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    TextureCapture capture(gpu.device);

    int frame_id = 0;
    while (!glfwWindowShouldClose(window)) {
        std::vector<CommandBuffer> commands;

        //glfwPollEvents();
        glfwWaitEvents();

        Surface surface;
        SurfaceTexture surface_info;
        if (!get_window_surface(gpu, window, surface, surface_info)) {
            std::cout << "surface texture not available\n";
            continue;
        }

        RenderPassColorAttachment attachment{
            .view = surface_info.texture.CreateView(),
            .loadOp = LoadOp::Clear,
            .storeOp = StoreOp::Store
        };

        RenderPassDescriptor render_pass{
            .colorAttachmentCount = 1,
            .colorAttachments = &attachment
        };

        CommandEncoder encoder = gpu.device.CreateCommandEncoder();
        RenderPassEncoder pass = encoder.BeginRenderPass(&render_pass);
        pass.SetPipeline(pipeline);
        pass.Draw(3);
        pass.End();

        commands.push_back(encoder.Finish());
        gpu.device.GetQueue().Submit(commands.size(), commands.data());

#if 1
        imgui.begin_frame(surface_info.texture.GetWidth(), surface_info.texture.GetHeight());

        if (ImGui::BeginMenu("File", true))
        {
            if (ImGui::MenuItem("Open asset...", nullptr, false))
            {
            }

            if (ImGui::MenuItem("Import image...", nullptr, nullptr))
            {
            }
            ImGui::EndMenu();
        }

        ImGui::ShowDemoWindow();

        imgui.end_frame(surface_info.texture);
#endif

        capture.push(surface_info.texture, nullptr,
            [frame_id](char const * image_data, ImageDataLayout const& layout) {
               std::stringstream file_name;
               file_name << "capture_" << std::setfill('0') << std::setw(4) << std::to_string(frame_id) << ".ppm";

               std::ofstream file(file_name.str());
               file << "P6\n" << layout.width << " " << layout.height << "\n" << 255 << "\n";
               for (int row = 0; row <layout.height; row++) {
                    char const * pixel = image_data + row * layout.row_stride;
                    for (int col = 0; col <layout.width; col++, pixel += 4) {
                        if (layout.format == TextureFormat::BGRA8Unorm) {
                            char rgb[3] = { *(pixel + 2), *(pixel + 1), *pixel };
                            file.write(rgb, 3);
                        } else {
                            file.write(pixel, 3);
                        }
                   }
               }
        });

        surface.Present();

        // Poll for and process events
        gpu.instance.ProcessEvents();

        capture.pop();

        // simple fps counter
        static int frame_count = 0;
        static auto prev = std::chrono::high_resolution_clock::now();
        if (frame_count >= 60) {
            auto now = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::micro> mean_us = now - prev;
            std::cout << "Mean fps = " << float(frame_count) / (mean_us.count() * 1e-6) << "\n";
            prev = now;
            frame_count = 1;
        } else {
            frame_count++;
        }

        frame_id++;
    }

    gpu.release();
    glfwTerminate();

    return 0;
}
