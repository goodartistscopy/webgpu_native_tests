#pragma once

#include <webgpu/webgpu_cpp.h>
#include <dawn/webgpu_cpp_print.h>

#include <cassert>
#include <iostream>
#include <functional>
//#include <type_traits>

uint32_t texture_format_size(wgpu::TextureFormat format);

bool is_srgb(wgpu::TextureFormat format);

wgpu::ImageCopyBuffer image_copy_buffer_for_texture(const wgpu::Texture & texture, uint64_t & size);

template<typename I1, typename I2 = I1
//         ,std::enable_if_t<std::is_unsigned<I>::value, bool> = true
>
I1 align_up(I1 value, I2 factor) {
    assert(factor > 0 && ((factor & (factor - 1)) == 0));
    return (value + factor - 1) & ~(factor - 1);
};

struct ImageDataLayout {
    wgpu::TextureFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t row_stride;
};

struct TextureCapture {
    TextureCapture(wgpu::Device device)
    : device(device) {}

    template<typename L>
    void push(wgpu::Texture const & texture, wgpu::CommandEncoder encoder, L callback) {
        if (this->head == this->tail) {
            std::cout << "Buffer queue is full\n";
            return;
        }

        Capture& capture = this->captures[this->head];
        this->head = (this->head + 1) % this->count;
        if (capture.buffer && capture.buffer.GetMapState() != wgpu::BufferMapState::Unmapped) {
            std::cout << "Buffer already mapped (state: " << capture.buffer.GetMapState() <<"\n";
            return;
        }

        capture.callback = callback;

        encode_texture_copy(texture, capture, encoder);
    }

    void pop();

    private:
        struct Capture {
            wgpu::Buffer buffer;
            ImageDataLayout layout;
            std::function<void(char const*, ImageDataLayout const& layout)> callback;
            wgpu::Future future;
        };

        void encode_texture_copy(wgpu::Texture const & texture, Capture & capture, wgpu::CommandEncoder encoder);

        Capture captures[2];
        uint32_t count = 2;
        uint32_t head = 1;
        uint32_t tail = 0;
        wgpu::Device device;
};
