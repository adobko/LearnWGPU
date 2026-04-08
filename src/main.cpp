#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
#else
    #include <dawn/webgpu_cpp_print.h>
    #include <webgpu/webgpu_glfw.h>
#endif
#include <webgpu/webgpu_cpp.h>

#include "cube.hpp"
#include "app.hpp"
#include "camera.hpp"

void handleMovemetInput(GLFWwindow* window);
void handleMousePos(GLFWwindow* window, double xpos, double ypos);
void handleMouseWheel(GLFWwindow* window, double xoffset, double yoffset);
std::string loadShader(const char* path);
wgpu::Texture loadTexture(const char* path);
void createBuffers();
void createBindGroup();
void createRenderPipeline();
void render();
void startRenderLoop();



wgpu::Buffer         vertexBuffer     = nullptr;
wgpu::Buffer         indexBuffer      = nullptr;
wgpu::Buffer         uniformBuffer    = nullptr;
wgpu::BindGroupLayout bindGroupLayout = nullptr;
wgpu::BindGroup      bindGroup        = nullptr;

float currentTime;
float lastTime;
float deltaTime;

static App app(
    800, 600, "WebGPU",
    []() {
        createBuffers();
        createBindGroup();
        createRenderPipeline();
        startRenderLoop();
    }
);

static Camera cam(
    glm::vec3(0.0f, 0.0f, 5.0f),
    glm::vec3(0.0f, 1.0f, 0.0f)
);

int main() {
    // Setup Camera
    glfwSetInputMode(app.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(app.window, handleMousePos);
    glfwSetScrollCallback(app.window, handleMouseWheel);

    // Get Device, setup render pipeline and kick off render loop
    app.initDeviceAndRun();
    return 0;
}

void handleMovemetInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.move(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.move(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.move(CameraMovement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.move(CameraMovement::RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cam.move(CameraMovement::UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cam.move(CameraMovement::DOWN, deltaTime);
    
}

void handleMousePos(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    glm::vec2 newMousePos(
        static_cast<float>(xpos), 
        static_cast<float>(ypos)
    );

    if (firstMouse) {
        cam.mousePos.x = app.wWidth / 2;
        cam.mousePos.y = app.wHeight / 2;
        firstMouse= false;
    }

    cam.rotate(glm::vec2(
        newMousePos.x - cam.mousePos.x, 
        cam.mousePos.y - newMousePos.y
    ));
    cam.mousePos = newMousePos;
}

void handleMouseWheel(GLFWwindow* window, double xoffset, double yoffset) {
    cam.zoom(yoffset);
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

wgpu::Texture loadTexture(const char* path) {
    int w, h, ch;
    u_int8_t* image = stbi_load(path, &w, &h, &ch, STBI_rgb_alpha);
    if (!image) {
        std::cerr << "Could not load texture: " << stbi_failure_reason() << std::endl;
        exit(1);
    }
    uint32_t width = static_cast<uint32_t>(w);
    uint32_t height = static_cast<uint32_t>(h);

    wgpu::TextureDescriptor texDesc{
        .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
        .dimension       = wgpu::TextureDimension::e2D,
        .size = { .width = width, .height = height, .depthOrArrayLayers = 1 },
        .format = wgpu::TextureFormat::RGBA8Unorm,
    };
    
    wgpu::Texture tex = app.device.CreateTexture(&texDesc);
    
    wgpu::TexelCopyTextureInfo texDest{ 
        .texture = tex, 
        .mipLevel = 0,
        .origin   = { 0, 0, 0 },
        .aspect   = wgpu::TextureAspect::All,
    };
    
    wgpu::TexelCopyBufferLayout dataLayout{
        .offset = 0,
        .bytesPerRow = 4 * width, // R, G, B, A -> (4 bytes)
        .rowsPerImage = height,
    };

    wgpu::Extent3D writeSize{
        .width = width,
        .height = height,
        .depthOrArrayLayers = 1,
    };

    app.device.GetQueue().WriteTexture(
        &texDest, 
        image, 
        4 * width * height, 
        &dataLayout, 
        &writeSize
    );
    
    stbi_image_free(image);
    return tex;
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
    lastTime = currentTime;
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    handleMovemetInput(app.window);

    // ── Build MVP matrix with GLM ─────────────────────────────────────────────
    float time = static_cast<float>(glfwGetTime());

    glm::mat4 model = glm::rotate(
        glm::mat4(1.0f), 
        time, 
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 view  = cam.getViewMatrix();

    glm::mat4 proj  = glm::perspective(
        glm::radians(cam.fov),        
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