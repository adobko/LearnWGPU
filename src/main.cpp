#include <iostream>
#include <fstream>
#include <string>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
#else
    #include <dawn/webgpu_cpp_print.h>
    #include <webgpu/webgpu_glfw.h>
#endif
#include <webgpu/webgpu_cpp.h>

#include "app.hpp"

std::string loadShader();
void createBuffers();
void createBindGroup();
void createRenderPipeline();
void render();
void startRenderLoop();

// clang-format off
const float vertices[] = {
    -0.5f, -0.5f, -0.5f,  // 0 back-bottom-left
     0.5f, -0.5f, -0.5f,  // 1 back-bottom-right
     0.5f,  0.5f, -0.5f,  // 2 back-top-right
    -0.5f,  0.5f, -0.5f,  // 3 back-top-left
    -0.5f, -0.5f,  0.5f,  // 4 front-bottom-left
     0.5f, -0.5f,  0.5f,  // 5 front-bottom-right
     0.5f,  0.5f,  0.5f,  // 6 front-top-right
    -0.5f,  0.5f,  0.5f,  // 7 front-top-left
};

const uint16_t indices[] = {
    0, 2, 1,  0, 3, 2,  // back
    4, 5, 6,  4, 6, 7,  // front
    0, 4, 7,  0, 7, 3,  // left
    1, 2, 6,  1, 6, 5,  // right
    0, 1, 5,  0, 5, 4,  // bottom
    3, 7, 6,  3, 6, 2,  // top
};

const uint32_t indexCount = sizeof(indices) / sizeof(indices[0]);

wgpu::Buffer         vertexBuffer     = nullptr;
wgpu::Buffer         indexBuffer      = nullptr;
wgpu::Buffer         uniformBuffer    = nullptr;
wgpu::BindGroupLayout bindGroupLayout = nullptr;
wgpu::BindGroup      bindGroup        = nullptr;

static App app(
    800, 600, "WebGPU",
    []() {
        createBuffers();
        createBindGroup();
        createRenderPipeline();
        startRenderLoop();
    }
);

int main() {
    app.initDeviceAndRun();
    return 0;
}

std::string loadShader(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Shader path could not be opened: " << path << std::endl;
        exit(1);
    }
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

void createBuffers() {
    wgpu::BufferDescriptor vbDesc{
        .label = "vertex buffer",
        .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
        .size  = sizeof(vertices),
    };
    vertexBuffer = app.device.CreateBuffer(&vbDesc);
    app.device.GetQueue().WriteBuffer(vertexBuffer, 0, vertices, sizeof(vertices));

    wgpu::BufferDescriptor ibDesc{
        .label = "index buffer",
        .usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst,
        .size  = sizeof(indices),
    };
    indexBuffer = app.device.CreateBuffer(&ibDesc);
    app.device.GetQueue().WriteBuffer(indexBuffer, 0, indices, sizeof(indices));

    wgpu::BufferDescriptor ubDesc{
        .label = "uniform buffer (MVP)",
        .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
        .size  = 3 * 256, // minUniformBufferOffsetAlignment = 256
    };
    uniformBuffer = app.device.CreateBuffer(&ubDesc);
    // We do not write this buffer yet as it dynamically canges each frame
}

void createBindGroup() {
    wgpu::BindGroupLayoutEntry layoutEntry{
        .binding    = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer     = {
            .type             = wgpu::BufferBindingType::Uniform,
            .hasDynamicOffset = true,
            .minBindingSize   = sizeof(glm::mat4),
        },
    };
    wgpu::BindGroupLayoutDescriptor bglDesc{
        .entryCount = 1,
        .entries    = &layoutEntry,
    };
    bindGroupLayout = app.device.CreateBindGroupLayout(&bglDesc);

    wgpu::BindGroupEntry bgEntry{
        .binding = 0,
        .buffer  = uniformBuffer,
        .offset  = 0,
        .size    = sizeof(glm::mat4),
    };
    wgpu::BindGroupDescriptor bgDesc{
        .layout     = bindGroupLayout,
        .entryCount = 1,
        .entries    = &bgEntry,
    };
    bindGroup = app.device.CreateBindGroup(&bgDesc);
}

void createRenderPipeline() {
    std::string shaderSource = loadShader("./src/shaders/shader.wgsl");
    wgpu::ShaderSourceWGSL wgsl{{ .code = shaderSource.c_str() }};
    wgpu::ShaderModuleDescriptor smDesc{ .nextInChain = &wgsl };
    wgpu::ShaderModule shader = app.device.CreateShaderModule(&smDesc);

    wgpu::VertexAttribute posAttr{
        .format         = wgpu::VertexFormat::Float32x3,
        .offset         = 0,
        .shaderLocation = 0,
    };
    wgpu::VertexBufferLayout vertLayout{
        .arrayStride    = 3 * sizeof(float),
        .attributeCount = 1,
        .attributes     = &posAttr,
    };

    wgpu::DepthStencilState depthStencil{
        .format            = wgpu::TextureFormat::Depth24Plus,
        .depthWriteEnabled = wgpu::OptionalBool::True,
        .depthCompare      = wgpu::CompareFunction::Less,
    };

    wgpu::PipelineLayoutDescriptor plDesc{
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts     = &bindGroupLayout,
    };
    wgpu::PipelineLayout pipelineLayout = app.device.CreatePipelineLayout(&plDesc);

    wgpu::ColorTargetState colorTarget{ .format = app.format };
    wgpu::FragmentState fragState{
        .module      = shader,
        .entryPoint = "fragmentMain",
        .targetCount = 1,
        .targets     = &colorTarget,
    };
    wgpu::RenderPipelineDescriptor pipeDesc{
        .layout  = pipelineLayout,
        .vertex  = {
            .module      = shader,
            .entryPoint = "vertexMain",
            .bufferCount = 1,
            .buffers     = &vertLayout,
        },
        .depthStencil = &depthStencil,
        .fragment     = &fragState,
    };
    app.pipeline = app.device.CreateRenderPipeline(&pipeDesc);
}

void render() {
#if !defined(__EMSCRIPTEN__)
    glfwPollEvents();
#endif

    // ── Build MVP matrix with GLM ─────────────────────────────────────────────
    float time = static_cast<float>(glfwGetTime());

    glm::mat4 model = glm::rotate(
        glm::mat4(1.0f), 
        time, 
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 view  = glm::lookAt(
        glm::vec3(0.0f, 1.0f, 2.0f),        
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 proj  = glm::perspective(
        glm::radians(60.0f),        
        static_cast<float>(app.wWidth) / static_cast<float>(app.wHeight),               
        0.1f, 10.0f
    );

    glm::mat4 mvp = proj * view * model;

    uint32_t offset0 = 0;
    uint32_t offset1 = 256;
    uint32_t offset2 = 2 * 256;

    app.device.GetQueue().WriteBuffer(uniformBuffer, offset0, &mvp, sizeof(mvp));

    glm::mat4 model2 = glm::translate(model, glm::vec3(0.5f, 0.5f, 0.5f));
    model2 = glm::scale(model2, glm::vec3(0.5f, 0.5f, 0.5f));
    glm::mat4 mvp2 = proj * view * model2;
    app.device.GetQueue().WriteBuffer(uniformBuffer, offset1, &mvp2, sizeof(mvp2));

    glm::mat4 model3 = glm::translate(model, glm::vec3(-0.5f, -0.5f, -0.5f));
    model2 = glm::scale(model2, glm::vec3(0.75f, 0.75f, 0.75f));
    glm::mat4 mvp3 = proj * view * model3;
    app.device.GetQueue().WriteBuffer(uniformBuffer, offset2, &mvp3, sizeof(mvp3));

    // ── Draw ──────────────────────────────────────────────────────────────────
    wgpu::SurfaceTexture surfaceTexture;
    app.surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment colorAttachment{
        .view    = surfaceTexture.texture.CreateView(),
        .loadOp  = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
    };

    wgpu::RenderPassDepthStencilAttachment depthAttachment{
        .view            = app.depthTexture.CreateView(),
        .depthLoadOp     = wgpu::LoadOp::Clear,
        .depthStoreOp    = wgpu::StoreOp::Store,
        .depthClearValue = 1.0f,
    };

    wgpu::RenderPassDescriptor passDesc{
        .colorAttachmentCount   = 1,
        .colorAttachments       = &colorAttachment,
        .depthStencilAttachment = &depthAttachment,
    };

    wgpu::CommandEncoder encoder = app.device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
    
    pass.SetPipeline(app.pipeline);
    pass.SetVertexBuffer(0, vertexBuffer);
    pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint16);
    
    pass.SetBindGroup(0, bindGroup, 1, &offset0);
    pass.DrawIndexed(indexCount);

    pass.SetBindGroup(0, bindGroup, 1, &offset1);
    pass.DrawIndexed(indexCount);

    pass.SetBindGroup(0, bindGroup, 1, &offset2);
    pass.DrawIndexed(indexCount);

    pass.End();

    wgpu::CommandBuffer commands = encoder.Finish();
    app.device.GetQueue().Submit(1, &commands);

#if !defined(__EMSCRIPTEN__)
    app.surface.Present();
    app.instance.ProcessEvents();
#endif
}

void startRenderLoop() {
#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(render, 0, false);
#else
    while (!glfwWindowShouldClose(app.window)) {
        render();
    }
#endif
}