#include "webgpu_helpers.h"

#include <cassert>

uint32_t texture_format_size(wgpu::TextureFormat format) {
    switch (format) {
        case wgpu::TextureFormat::R8Unorm:
        case wgpu::TextureFormat::R8Snorm:
        case wgpu::TextureFormat::R8Uint:
        case wgpu::TextureFormat::R8Sint:
            return 1;
        case wgpu::TextureFormat::R16Uint:
        case wgpu::TextureFormat::R16Sint:
        case wgpu::TextureFormat::R16Float:
        case wgpu::TextureFormat::RG8Unorm:
        case wgpu::TextureFormat::RG8Snorm:
        case wgpu::TextureFormat::RG8Uint:
        case wgpu::TextureFormat::RG8Sint:
            return 2;
        case wgpu::TextureFormat::R32Float:
        case wgpu::TextureFormat::R32Uint:
        case wgpu::TextureFormat::R32Sint:
        case wgpu::TextureFormat::RG16Uint:
        case wgpu::TextureFormat::RG16Sint:
        case wgpu::TextureFormat::RG16Float:
        case wgpu::TextureFormat::RGBA8Unorm:
        case wgpu::TextureFormat::RGBA8UnormSrgb:
        case wgpu::TextureFormat::RGBA8Snorm:
        case wgpu::TextureFormat::RGBA8Uint:
        case wgpu::TextureFormat::RGBA8Sint:
        case wgpu::TextureFormat::BGRA8Unorm:
        case wgpu::TextureFormat::BGRA8UnormSrgb:
        case wgpu::TextureFormat::RGB10A2Uint:
        case wgpu::TextureFormat::RGB10A2Unorm:
        case wgpu::TextureFormat::RG11B10Ufloat:
        case wgpu::TextureFormat::RGB9E5Ufloat:
            return 4;
        case wgpu::TextureFormat::RG32Float:
        case wgpu::TextureFormat::RG32Uint:
        case wgpu::TextureFormat::RG32Sint:
        case wgpu::TextureFormat::RGBA16Uint:
        case wgpu::TextureFormat::RGBA16Sint:
        case wgpu::TextureFormat::RGBA16Float:
            return 8;
        case wgpu::TextureFormat::RGBA32Float:
        case wgpu::TextureFormat::RGBA32Uint:
        case wgpu::TextureFormat::RGBA32Sint:
            return 16;
        case wgpu::TextureFormat::Stencil8:
            return 8;
        case wgpu::TextureFormat::Depth16Unorm:
            return 2;
        case wgpu::TextureFormat::Depth24Plus:
            assert(false && "unimplemented");
            return 0;
        case wgpu::TextureFormat::Depth24PlusStencil8:
            assert(false && "unimplemented");
            return 0;
        case wgpu::TextureFormat::Depth32Float:
            return 4;
        case wgpu::TextureFormat::Depth32FloatStencil8:
            assert(false && "unimplemented");
            return 0;
        case wgpu::TextureFormat::BC1RGBAUnorm:
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
        case wgpu::TextureFormat::BC2RGBAUnorm:
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
        case wgpu::TextureFormat::BC3RGBAUnorm:
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
        case wgpu::TextureFormat::BC4RUnorm:
        case wgpu::TextureFormat::BC4RSnorm:
        case wgpu::TextureFormat::BC5RGUnorm:
        case wgpu::TextureFormat::BC5RGSnorm:
        case wgpu::TextureFormat::BC6HRGBUfloat:
        case wgpu::TextureFormat::BC6HRGBFloat:
        case wgpu::TextureFormat::BC7RGBAUnorm:
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
        case wgpu::TextureFormat::ETC2RGB8Unorm:
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
        case wgpu::TextureFormat::ETC2RGB8A1Unorm:
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
        case wgpu::TextureFormat::ETC2RGBA8Unorm:
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
        case wgpu::TextureFormat::EACR11Unorm:
        case wgpu::TextureFormat::EACR11Snorm:
        case wgpu::TextureFormat::EACRG11Unorm:
        case wgpu::TextureFormat::EACRG11Snorm:
        case wgpu::TextureFormat::ASTC4x4Unorm:
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
        case wgpu::TextureFormat::ASTC5x4Unorm:
        case wgpu::TextureFormat::ASTC5x4UnormSrgb:
        case wgpu::TextureFormat::ASTC5x5Unorm:
        case wgpu::TextureFormat::ASTC5x5UnormSrgb:
        case wgpu::TextureFormat::ASTC6x5Unorm:
        case wgpu::TextureFormat::ASTC6x5UnormSrgb:
        case wgpu::TextureFormat::ASTC6x6Unorm:
        case wgpu::TextureFormat::ASTC6x6UnormSrgb:
        case wgpu::TextureFormat::ASTC8x5Unorm:
        case wgpu::TextureFormat::ASTC8x5UnormSrgb:
        case wgpu::TextureFormat::ASTC8x6Unorm:
        case wgpu::TextureFormat::ASTC8x6UnormSrgb:
        case wgpu::TextureFormat::ASTC8x8Unorm:
        case wgpu::TextureFormat::ASTC8x8UnormSrgb:
        case wgpu::TextureFormat::ASTC10x5Unorm:
        case wgpu::TextureFormat::ASTC10x5UnormSrgb:
        case wgpu::TextureFormat::ASTC10x6Unorm:
        case wgpu::TextureFormat::ASTC10x6UnormSrgb:
        case wgpu::TextureFormat::ASTC10x8Unorm:
        case wgpu::TextureFormat::ASTC10x8UnormSrgb:
        case wgpu::TextureFormat::ASTC10x10Unorm:
        case wgpu::TextureFormat::ASTC10x10UnormSrgb:
        case wgpu::TextureFormat::ASTC12x10Unorm:
        case wgpu::TextureFormat::ASTC12x10UnormSrgb:
        case wgpu::TextureFormat::ASTC12x12Unorm:
        case wgpu::TextureFormat::ASTC12x12UnormSrgb:
            assert(false && "unimplemented");
            return 0;
        case wgpu::TextureFormat::R16Unorm:
            return 2;
        case wgpu::TextureFormat::RG16Unorm:
        case wgpu::TextureFormat::RGBA16Unorm:
        case wgpu::TextureFormat::R16Snorm:
        case wgpu::TextureFormat::RG16Snorm:
            return 4;
        case wgpu::TextureFormat::RGBA16Snorm:
            return 8;
        case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
        case wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm:
        case wgpu::TextureFormat::R8BG8A8Triplanar420Unorm:
        case wgpu::TextureFormat::R8BG8Biplanar422Unorm:
        case wgpu::TextureFormat::R8BG8Biplanar444Unorm:
        case wgpu::TextureFormat::R10X6BG10X6Biplanar422Unorm:
        case wgpu::TextureFormat::R10X6BG10X6Biplanar444Unorm:
            assert(false && "unimplemented");
            return 0;
        default:
            return 0;
    }
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

uint64_t get_buffer_size_for_texture_copy(wgpu::Texture texture, uint32_t& row_stride) {
    row_stride = align_up<uint32_t>(texture.GetWidth() * texture_format_size(texture.GetFormat()), 256);
    return row_stride * texture.GetHeight();
}

wgpu::ImageCopyBuffer image_copy_buffer_for_texture(const wgpu::Texture & texture, uint64_t & size) {
    uint32_t row_stride = align_up<uint32_t>(texture.GetWidth() * texture_format_size(texture.GetFormat()), 256);
    size = row_stride * texture.GetHeight();
    wgpu::ImageCopyBuffer icb{
        .layout = {
            .offset = 0,
            .bytesPerRow = row_stride,
            .rowsPerImage = texture.GetHeight(),
        },
        .buffer = nullptr,
    };
    return icb;
}

void TextureCapture::pop() {
    Capture& capture = this->captures[this->tail];

    if (capture.buffer && capture.buffer.GetMapState() == wgpu::BufferMapState::Unmapped) {
        wgpu::FutureWaitInfo read_back_info;
        read_back_info.future = capture.buffer.MapAsync(wgpu::MapMode::Read, 0, capture.buffer.GetSize(), wgpu::CallbackMode::WaitAnyOnly,
            [&capture](wgpu::MapAsyncStatus status, char const * message) {
                if (status != wgpu::MapAsyncStatus::Success) {
                    std::cout << "Buffer mapping error: " << status << "\n";
                    return;
                }

                char const * data = reinterpret_cast<char const *>(capture.buffer.GetConstMappedRange());
                capture.callback(data, capture.layout);

                capture.buffer.Unmap();
            }
        );
        auto status = this->device.GetAdapter().GetInstance().WaitAny(1, &read_back_info, 0);
        if (status != wgpu::WaitStatus::Success || !read_back_info.completed) {
            std::cout << "WaitAny failure: " << status << ". Buffer status = " << capture.buffer.GetMapState() << "\n";
            return; // don't advance just yet
        }
    }

    this->tail = (this->tail + 1) % this->count;
}

void TextureCapture::encode_texture_copy(wgpu::Texture const & texture, Capture & capture, wgpu::CommandEncoder encoder) {
    uint64_t required_size;
    wgpu::ImageCopyBuffer destination = image_copy_buffer_for_texture(texture, required_size);

    uint64_t buffer_size = (capture.buffer) ? capture.buffer.GetSize() : 0;
    if (buffer_size < required_size) {
        wgpu::BufferDescriptor desc{
            .label = "texture capture",
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
            .size = required_size,
        };
        capture.buffer = device.CreateBuffer(&desc);
    }
    assert(capture.buffer);
    destination.buffer = capture.buffer;

    wgpu::ImageCopyTexture source{
        .texture = texture,
    };

    wgpu::Extent3D extent{
        .width = texture.GetWidth(),
        .height = texture.GetHeight(),
    };

    bool submit = false;
    if (!encoder) {
        encoder = this->device.CreateCommandEncoder();
        submit = true;
    }

    encoder.CopyTextureToBuffer(&source, &destination, &extent);

    if (submit) {
        wgpu::CommandBuffer cmds = encoder.Finish();
        this->device.GetQueue().Submit(1, &cmds);
    }

    capture.layout = ImageDataLayout{
      .format = texture.GetFormat(),
      .width = texture.GetWidth(),
      .height = texture.GetHeight(),
      .row_stride = destination.layout.bytesPerRow,
    };
}
