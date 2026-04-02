#include <iostream>
#include <GLFW/glfw3.h>

#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
#else
    #include <dawn/webgpu_cpp_print.h>
    #include <webgpu/webgpu_glfw.h>
#endif

#include <webgpu/webgpu_cpp.h>

struct App {
    GLFWwindow*          window   = nullptr;
    wgpu::Instance       instance = nullptr;
    wgpu::Adapter        adapter  = nullptr;
    wgpu::Surface        surface  = nullptr;
    wgpu::Device         device   = nullptr;
    wgpu::RenderPipeline pipeline = nullptr;
    wgpu::TextureFormat  format   = wgpu::TextureFormat::BGRA8Unorm;
    const int wWidth = 800, wHeight = 600;
    const char* wTitle = (char*)"WebGPU";
};

static App app;

const char shaderSource[] = R"(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
        const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
        return vec4f(pos[i], 0, 1);
    }
    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1, 0, 1, 1);
    }
)";

void configureSurface();
void createRenderPipeline();
void startRenderLoop();
inline void onDeviceReady();

void initGLFW() {
    if (!glfwInit()) {
        std::cerr << "glfwInit failed" << std::endl;
        exit(1);
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#if !defined(__EMSCRIPTEN__)
    glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, app.wTitle);
#endif
    app.window = glfwCreateWindow(app.wWidth, app.wHeight, app.wTitle, nullptr, nullptr);
}

void initWGPU() {
#if defined(__EMSCRIPTEN__)
    app.instance = wgpu::CreateInstance(nullptr);
#else
    static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor desc{
        .requiredFeatureCount = 1,
        .requiredFeatures     = &kTimedWaitAny,
    };
    app.instance = wgpu::CreateInstance(&desc);
#endif
}

void initSurface() {
#if defined(__EMSCRIPTEN__)
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";
    wgpu::SurfaceDescriptor surfDesc{.nextInChain = &canvasDesc};
    app.surface = app.instance.CreateSurface(&surfDesc);
#else
    app.surface = wgpu::glfw::CreateSurfaceForWindow(app.instance, app.window);
#endif
}

void initAdapterAndDevice() {
#if defined(__EMSCRIPTEN__)
    static const auto initDevice = []() {
        wgpu::DeviceDescriptor devDesc{};
        devDesc.SetUncapturedErrorCallback(
            [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
                std::cerr << "WebGPU error " << static_cast<int>(type)
                        << ": " << msg.data << std::endl;
            });

        app.adapter.RequestDevice(
            &devDesc,
            wgpu::CallbackMode::AllowSpontaneous,
            [](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView msg) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    std::cerr << "RequestDevice failed: " << msg.data << std::endl;
                    exit(1);
                }
                app.device = std::move(d);
                onDeviceReady();
            }
        );
    }; 
    wgpu::RequestAdapterOptions opts{ .compatibleSurface = app.surface };
    
    app.instance.RequestAdapter(
        &opts,
        wgpu::CallbackMode::AllowSpontaneous,
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView msg) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cerr << "RequestAdapter failed: " << msg.data << std::endl;
                exit(1);
            }
            app.adapter = std::move(a);
            initDevice();
        }
    );
#else
    wgpu::RequestAdapterOptions opts{ .compatibleSurface = app.surface };

    wgpu::Future f1 = app.instance.RequestAdapter(
        &opts,
        wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView msg) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cerr << "RequestAdapter failed: " << msg.data << std::endl;
                exit(1);
            }
            app.adapter = std::move(a);
        }
    );
    app.instance.WaitAny(f1, UINT64_MAX);

    wgpu::DeviceDescriptor devDesc{};
    devDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
            std::cerr << "WebGPU error " << type
                      << ": " << msg.data << std::endl;
        }
    );

    wgpu::Future f2 = app.adapter.RequestDevice(
        &devDesc,
        wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView msg) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::cerr << "RequestDevice failed: " << msg.data << std::endl;
                exit(1);
            }
            app.device = std::move(d);
        }
    );
    app.instance.WaitAny(f2, UINT64_MAX);
    
    onDeviceReady();
#endif
}

void configureSurface() {
    wgpu::SurfaceCapabilities cap;
    app.surface.GetCapabilities(app.adapter, &cap);
    app.format = cap.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = app.device,
        .format = app.format,
        .width  = static_cast<uint32_t>(app.wWidth),
        .height = static_cast<uint32_t>(app.wHeight),
    };
    app.surface.Configure(&config);
}

void createRenderPipeline() {
    wgpu::ShaderSourceWGSL wgsl{{.code = shaderSource}};
    wgpu::ShaderModuleDescriptor smDesc{.nextInChain = &wgsl};
    wgpu::ShaderModule shader = app.device.CreateShaderModule(&smDesc);

    wgpu::ColorTargetState colorTarget{.format = app.format};
    wgpu::FragmentState fragState{
        .module      = shader,
        .targetCount = 1,
        .targets     = &colorTarget,
    };
    wgpu::RenderPipelineDescriptor pipeDesc{
        .vertex   = {.module = shader},
        .fragment = &fragState,
    };
    app.pipeline = app.device.CreateRenderPipeline(&pipeDesc);
}

void render() {
#if !defined(__EMSCRIPTEN__)
    glfwPollEvents();
#endif

    wgpu::SurfaceTexture surfaceTexture;
    app.surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment{
        .view    = surfaceTexture.texture.CreateView(),
        .loadOp  = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
    };
    wgpu::RenderPassDescriptor passDesc{
        .colorAttachmentCount = 1,
        .colorAttachments     = &attachment,
    };

    wgpu::CommandEncoder encoder = app.device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
    pass.SetPipeline(app.pipeline);
    pass.Draw(3);
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

inline void onDeviceReady() {
    configureSurface();
    createRenderPipeline();
    startRenderLoop();
}

int main() {
    // Initialize a GLFW window
    initGLFW();
    // Initialize a WGPU instance
    initWGPU();
    // Initialize a WGPU surface in the GLFW window
    initSurface();
    // Initialize a WGPU adapter, device, setup the surface and render pipeline and start render loop
    initAdapterAndDevice();
    return 0;
}