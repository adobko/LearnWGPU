#include <iostream>

#include <GLFW/glfw3.h>
#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
#endif
#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

wgpu::Instance instance;
wgpu::Adapter adapter;
wgpu::Device device;
GLFWwindow *window;
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
wgpu::Surface surface;
wgpu::TextureFormat format;
wgpu::RenderPipeline pipeline;

void initWgpu() {
    static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor desc = {
        .requiredFeatureCount=1,
        .requiredFeatures=&kTimedWaitAny,
    };
    instance = wgpu::CreateInstance(&desc);
}

void initAdapter() {
    // wgpu::RequestAdapterOptions options{
    //     .nextInChain=nullptr,
    //     .featureLevel=wgpu::FeatureLevel::Compatibility,
    //     .powerPreference=wgpu::PowerPreference::HighPerformance,
    //     .forceFallbackAdapter=false,
    //     .backendType=wgpu::BackendType::Vulkan,
    //     .compatibleSurface=nullptr
    // };
    wgpu::Future f = instance.RequestAdapter(
        nullptr/*&options*/, 
        wgpu::CallbackMode::WaitAnyOnly, 
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message){
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cerr << "Adapter: " << message;
                exit(1);
            }
            adapter = std::move(a);
        }
    );
    instance.WaitAny(f, UINT64_MAX);
}



void initDevice() {
    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message) {
            std::cout << "Error: " << errorType << " - message: " << message << "\n";
        }
    );
    wgpu::Future f = adapter.RequestDevice(
        &desc, 
        wgpu::CallbackMode::WaitAnyOnly, 
        [](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message){
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::cerr << "Device" << message;
                exit(1);
            }
            device = std::move(d);
        }
    );
    instance.WaitAny(f, UINT64_MAX);
}

void initGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed ot initialize GLFW!";
        exit(1);
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, "WebGPU");
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "WebGPU", nullptr, nullptr);
}

void initSurface() {
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
    wgpu::SurfaceCapabilities cap;
    surface.GetCapabilities(adapter, &cap);
    format = cap.formats[0];
    wgpu::SurfaceConfiguration config{
        .device=device,
        .format=format,
        .width=WINDOW_WIDTH,
        .height=WINDOW_HEIGHT,
    };
    surface.Configure(&config);
}

const char shaderSource[] = R"(
    @vertex fn vertexMain(@builtin(vertex_index) i : u32) -> @builtin(position) vec4f {
        const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
        return vec4f(pos[i], 0, 1);
    }
    
    @fragment fn fragmentMain() -> @location(0) vec4f {
        return vec4f(1,0,1,1);
    }
)";

void createRenderPipeline() {
    wgpu::ShaderSourceWGSL wgsl{{.code=shaderSource}};
    wgpu::ShaderModuleDescriptor desc1{.nextInChain=&wgsl};
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&desc1);
    wgpu::ColorTargetState colorTargetState{.format=format};
    wgpu::FragmentState fragState{
        .module=shaderModule,
        .targetCount=1,
        .targets=&colorTargetState,
    };
    wgpu::RenderPipelineDescriptor desc2{
        .vertex={.module=shaderModule},
        .fragment=&fragState,
    };
    
    pipeline = device.CreateRenderPipeline(&desc2);
}

void render() {
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);
    wgpu::RenderPassColorAttachment attachment{
        .view=surfaceTexture.texture.CreateView(),
        .loadOp=wgpu::LoadOp::Clear,
        .storeOp=wgpu::StoreOp::Store,
    };
    wgpu::RenderPassDescriptor desc{
        .colorAttachmentCount=1,
        .colorAttachments=&attachment,
    };
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&desc);
    pass.SetPipeline(pipeline);
    pass.Draw(3);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

int main() {
    initWgpu();
    initAdapter();
    initDevice();
    initGLFW();
    initSurface();

    createRenderPipeline();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(render, 0, false);
#else
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
        surface.Present();
        instance.ProcessEvents();
    }
#endif
    return 0;
}