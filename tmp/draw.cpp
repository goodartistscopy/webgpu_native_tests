#include <webgpu/webgpu_cpp.h>

using namespace wgpu;

struct Pass {
    union {
        wgpu::RenderPassDescriptor render;
        wgpu::ComputePassDesciptor compute;
    } desc;
    bool is_compute;
    std::vector<DrawCall> draw_calls;
}

struct Renderer {
    std::vector<Pass> passes;
};

struct Buffer {
    wgpu::Buffer buffer;
    uint32_t offset;
};

struct DrawCall {
    Handle<PSO> pipeline;
    Handle<wgpu::BindGroup> bind_groups[4];
    Handle<wgpu::Buffer> index_buffer;
    Handle<wgpu::Buffer> vertex_buffers[3];
    uint32_t index_offset = 0;
    uint32_t vertex_offset;
    uint32_t instance_offset = 0;
    uint32_t instance_count = 0;
    uint32_t buffer_offsets[2] = { 0 }; // look into that
    uint32_t triangle_count = 0;
};

struct Layout {
    wgpu::VertexBufferLayout layout;
}

struct Mesh {
    Handle<Buffer> index_buffer;
    Handle<Buffer> vertex_buffers[3];
    Handle<Layout> layout;
}

draw_call.set_shaders(shader_module);
draw_call.set_mesh(mesh);
draw_call.set();


void submit_draws(const Renderer* rdr) {

    DrawCall lastDraw;

    wgpu::CommandEncoder encoder = device->CreateCommandEncoder();

    for (auto& pass: rdr->passes) {

        if (pass.is_compute) {
            wgpu::ComputePassEncoder pass = encoder->BeginComputePass(&pass.desc.compute);

            // ...

        } else {
            wgpu::RenderPassEncoder pass_encoder = encoder->BeginRenderPass(&pass.desc.render);

            pass_encoder.SetViewport(rdr.viewport);

            for (auto& draw : pass.draw_calls) {

                const wgpu::RenderPipeline& pipeline = pipeline_cache.get_pipeline();

                pass_encoder.SetPipeline(pipeline);

                // Bind groups
                for (int i = 0; i < 4; i++) {
                    if (draw.bind_groups[i] != lastDraw.bind_groups[i]) {
                       pass_encoder.SetBindGroup(i, draw.bind_groups[i]);
                       last_draw.bind_groups[i] = draw.bind_groups[i];
                    }
                }

                for (int i = 0; i < 3; i++) {
                    if ((draw.vertex_buffers[i] != last_draw.vertex_buffers[i]) || (draw.vertex_offsets[i] != last_draw.vertex_offsets[i]) {
                        pass_encoder.SetVertexBuffer(draw.vertex_buffers[i].buffer, draw.vertex_buffers[i].offset);
                        last_draw.vertex_buffers = draw.vertex_buffers;
                    }
                }

                if ((draw.index_buffer != last_draw.index_buffer) || (draw.index_offset != last_draw.index_offset)) {
                    pass_encoder.SetIndexBuffer(draw.index_buffer.buffer, draw.index_buffer.offset);
                    last_draw.index_buffer = draw.index_buffer;
                }

                pass_encoder.DrawIndexed(draw.triangle_count * 3, draw.instance_count, draw.index_offset, draw.vertex_offset, draw.instance_offset);
            }
        }
    }
}

//Handle<Buffer> allocateVerteBuffer(const VertexLayout &layout) { ... }


struct Drawable {
    Handle<VertexBuffer> vertex_buffers[4];
    Handle<IndexBuffer> index_buffer;
    uint32_t vertex_offset;
    uint32_t index_offset;
};

struct InstancedDrawable {
    Drawable& base_drawable;
    Handle<VertexBuffer> instance_buffers[2];
    uint32_t instance_offset;
};

struct ShaderRuntime {
    wgpu::RenderPipeline pipeline;
    wgpu::BindGroup bind_groups[3];
};


wgpu::Buffer large_buffer = allocate_buffer(block_layout, num_blocks);
// -> BufferBinding for BindGroup
uint64_t offset = last_offset + block_layout.size;

wgpu::BindGroupEntry entry = {
    .binding = 0,
    .buffer = large_buffer,
    .offset = offset
};

wgpu::BindGroupDescriptor desc = {
    .layout = bind_group_layout
};

runtime->bind_group[0] = device->CreateBindGroup(desc);


int main() {    
    Renderer rdr;

    rdr.add_pass("opaque", pass_desc);
    rdr.add_pass("transparent", pass_desc);
    rdr.add_pass("bloom", pass_desc);

    auto opaquePass = rdr.getPass();

    for (const& drawable: drawables) {
        DrawCall dc;

        for (pass: rdr.get_passes()) {
            Shaders shaders = drawable.get_shaders(pass.get_name());

            vertex_layout = drawable.get_layout();

            dc.pipeline = pipeline_cache.get(shaders, vertex_layout, pass, state);
            

            dc.bind_groups[0] = drawable.shader_runtime.bind_group[0];


            pass.submitCall(dc);
        }
    }

    

}

struct GltfPBRMaterial {
    Handle<Texture> base_texture;
    float base_color[4];

    Handle<Texture> brdf_texture; // R: metalness, G: roughness: B: ??
    float metalness;
    float roughness;

    Handle<Texture> normal_texture;
    float normal_factor;

    Handle<Texture> emissive_texture;
    float emissive_color[4];

    float transmission;
};


struct Material {
    std::map<std::string, Handle<Shader>> shaders; // shaders["opaque"]
};


//============================================================

struct Node {
    mat4 xform;
    Handle<Entity> entity;
    int child;
};


struct Scene {
    Arena<Node> nodes;
};


Scene* scene = new Scene();

// root -> node1 -> node2
//      -> node3
Handle<Node> node1 = scene->add_node(entity, xform /* , nullptr */);
Handle<Node> node2 = scene->add_node(entity, xform, node1);
Handle<Node> node3 = scene->add_node(entity, xform, Handle<Node>::s_invalid);


class Arenas {
    static Arenas& getInstance();

    Handle<Arena> createArena();
    Arena<Arena> arenas;
}

template<typename Tag>
struct AutoHandle {
    uint32_t idx: 16;
    uint32_t arena_idx: 8;
    uint32_t generation: 8;
};

Node* node = node1.get();

if (Node* maybe_node1 = scene->get_node(node1)) {
    scene->remove_node(node1);
    // can work because the memory's still there in the arena (unless it can shrink)
    maybe_node1->xform = ...
}


struct Node {
    ...
    NodePtr child;
    Node* parent;
};

struct Scene {
    NodePtr root;
};

NodeWPtr node_hnd1 = scene->add_node(entity, xform);
NodeWPtr node_hnd2 = scene->add_node(entity, xform, node1);
NodeWPtr node_hnd3 = scene->add_node(entity, xform, nullptr);

if (NodePtr node1 = node_hnd1.lock()) {
    scene->remove_node(node_hnd1);
    // works but on a limbo node
    node1->xform = ...
}

struct Scene {
    Node* root;
};

Scene* scene = new Scene();
Node* node = scene->add_node(entity, xform, nullptr);
scene->remove_node(node);
//delete node
//node->xform


Scene* scene = new Scene();
WNode node = scene->add_node(entity, xform, nullptr);
scene->remove_node(node);

node.aquire()->xform ...
if (Node* node = node.aquire()) {
    node->xform = ...
}


Node* root = new Node(entity);
Node* node = new Node(entity1);
Node* node1 = new Node(entity1);
root->set_child(node);
root->set_child(node1);
delete node;

delete root;
