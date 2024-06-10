#pragma once

#include <webgpu/webgpu_cpp.h>

const char* to_string(wgpu::TextureFormat format);

bool is_srgb(wgpu::TextureFormat format);
