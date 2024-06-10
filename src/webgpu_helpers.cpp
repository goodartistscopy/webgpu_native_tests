#include "webgpu_helpers.h"

#include <cassert>

const char* to_string(wgpu::TextureFormat format) {
    const char* names[] = {   
        "Undefined",
        "R8Unorm",
        "R8Snorm",
        "R8Uint",
        "R8Sint",
        "R16Uint",
        "R16Sint",
        "R16Float",
        "RG8Unorm",
        "RG8Snorm",
        "RG8Uint",
        "RG8Sint",
        "R32Float",
        "R32Uint",
        "R32Sint",
        "RG16Uint",
        "RG16Sint",
        "RG16Float",
        "RGBA8Unorm",
        "RGBA8UnormSrgb",
        "RGBA8Snorm",
        "RGBA8Uint",
        "RGBA8Sint",
        "BGRA8Unorm",
        "BGRA8UnormSrgb",
        "RGB10A2Uint",
        "RGB10A2Unorm",
        "RG11B10Ufloat",
        "RGB9E5Ufloat",
        "RG32Float",
        "RG32Uint",
        "RG32Sint",
        "RGBA16Uint",
        "RGBA16Sint",
        "RGBA16Float",
        "RGBA32Float",
        "RGBA32Uint",
        "RGBA32Sint",
        "Stencil8",
        "Depth16Unorm",
        "Depth24Plus",
        "Depth24PlusStencil8",
        "Depth32Float",
        "Depth32FloatStencil8",
        "BC1RGBAUnorm",
        "BC1RGBAUnormSrgb",
        "BC2RGBAUnorm",
        "BC2RGBAUnormSrgb",
        "BC3RGBAUnorm",
        "BC3RGBAUnormSrgb",
        "BC4RUnorm",
        "BC4RSnorm",
        "BC5RGUnorm",
        "BC5RGSnorm",
        "BC6HRGBUfloat",
        "BC6HRGBFloat",
        "BC7RGBAUnorm",
        "BC7RGBAUnormSrgb",
        "ETC2RGB8Unorm",
        "ETC2RGB8UnormSrgb",
        "ETC2RGB8A1Unorm",
        "ETC2RGB8A1UnormSrgb",
        "ETC2RGBA8Unorm",
        "ETC2RGBA8UnormSrgb",
        "EACR11Unorm",
        "EACR11Snorm",
        "EACRG11Unorm",
        "EACRG11Snorm",
        "ASTC4x4Unorm",
        "ASTC4x4UnormSrgb",
        "ASTC5x4Unorm",
        "ASTC5x4UnormSrgb",
        "ASTC5x5Unorm",
        "ASTC5x5UnormSrgb",
        "ASTC6x5Unorm",
        "ASTC6x5UnormSrgb",
        "ASTC6x6Unorm",
        "ASTC6x6UnormSrgb",
        "ASTC8x5Unorm",
        "ASTC8x5UnormSrgb",
        "ASTC8x6Unorm",
        "ASTC8x6UnormSrgb",
        "ASTC8x8Unorm",
        "ASTC8x8UnormSrgb",
        "ASTC10x5Unorm",
        "ASTC10x5UnormSrgb",
        "ASTC10x6Unorm",
        "ASTC10x6UnormSrgb",
        "ASTC10x8Unorm",
        "ASTC10x8UnormSrgb",
        "ASTC10x10Unorm",
        "ASTC10x10UnormSrgb",
        "ASTC12x10Unorm",
        "ASTC12x10UnormSrgb",
        "ASTC12x12Unorm",
        "ASTC12x12UnormSrgb",
        "R16Unorm",
        "RG16Unorm",
        "RGBA16Unorm",
        "R16Snorm",
        "RG16Snorm",
        "RGBA16Snorm",
        "R8BG8Biplanar420Unorm",
        "R10X6BG10X6Biplanar420Unorm",
        "R8BG8A8Triplanar420Unorm",
        "R8BG8Biplanar422Unorm",
        "R8BG8Biplanar444Unorm",
        "R10X6BG10X6Biplanar422Unorm",
        "R10X6BG10X6Biplanar444Unorm",
        "External",
    };

    assert(static_cast<uint32_t>(format) < sizeof(names));

    return names[static_cast<uint32_t>(format)];
}

bool is_srgb(wgpu::TextureFormat format) {
    switch (format) {
        case wgpu::TextureFormat::RGBA8UnormSrgb:
        case wgpu::TextureFormat::BGRA8UnormSrgb:
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
        case wgpu::TextureFormat::ASTC5x4UnormSrgb:
        case wgpu::TextureFormat::ASTC5x5UnormSrgb:
        case wgpu::TextureFormat::ASTC6x5UnormSrgb:
        case wgpu::TextureFormat::ASTC6x6UnormSrgb:
        case wgpu::TextureFormat::ASTC8x5UnormSrgb:
        case wgpu::TextureFormat::ASTC8x6UnormSrgb:
        case wgpu::TextureFormat::ASTC8x8UnormSrgb:
        case wgpu::TextureFormat::ASTC10x5UnormSrgb:
        case wgpu::TextureFormat::ASTC10x6UnormSrgb:
        case wgpu::TextureFormat::ASTC10x8UnormSrgb:
        case wgpu::TextureFormat::ASTC10x10UnormSrgb:
        case wgpu::TextureFormat::ASTC12x10UnormSrgb:
        case wgpu::TextureFormat::ASTC12x12UnormSrgb:
            return true;
        default:
            return false;
    }
}
