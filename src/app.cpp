#include "app.hpp"
#include <iostream>
#include <GLFW/glfw3.h>
#if defined(__EMSCRIPTEN__)
    #include <emscripten/emscripten.h>
#else
    #include <dawn/webgpu_cpp_print.h>
    #include <webgpu/webgpu_glfw.h>
#endif
#include <webgpu/webgpu_cpp.h>

void App::initGLFW() {
    if (!glfwInit()) {
        std::cerr << "glfwInit failed" << std::endl;
        exit(1);
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#if !defined(__EMSCRIPTEN__)
    glfwWindowHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
    glfwWindowHintString(GLFW_WAYLAND_APP_ID, wTitle);
#endif
    window = glfwCreateWindow(
        wWidth, 
        wHeight, 
        wTitle,
        nullptr, 
        nullptr
    );
    glfwMakeContextCurrent(window);
}

void App::initWGPU() {
#if defined(__EMSCRIPTEN__)
    instance = wgpu::CreateInstance(nullptr);
#else
    static const auto kTimedWaitAny = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor desc{
        .requiredFeatureCount = 1,
        .requiredFeatures     = &kTimedWaitAny,
    };
    instance = wgpu::CreateInstance(&desc);
#endif
}

void App::initSurface() {
#if defined(__EMSCRIPTEN__)
    wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc{};
    canvasDesc.selector = "#canvas";
    wgpu::SurfaceDescriptor surfDesc{ .nextInChain = &canvasDesc };
    surface = instance.CreateSurface(&surfDesc);
#else
    surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#endif
}

void App::configureSurface() {
    wgpu::SurfaceCapabilities cap;
    surface.GetCapabilities(adapter, &cap);
    format = cap.formats[0];

    wgpu::SurfaceConfiguration config{
        .device = device,
        .format = format,
        .width  = static_cast<uint32_t>(wWidth),
        .height = static_cast<uint32_t>(wHeight),
    };
    surface.Configure(&config);
}

void App::createDepthTexture() {
    wgpu::TextureDescriptor depthDesc{
        .usage         = wgpu::TextureUsage::RenderAttachment,
        .dimension     = wgpu::TextureDimension::e2D,
        .size          = { static_cast<uint32_t>(wWidth),
                           static_cast<uint32_t>(wHeight), 1 },
        .format        = wgpu::TextureFormat::Depth24Plus,
        .mipLevelCount = 1,
        .sampleCount   = 1,
    };
    depthTexture = device.CreateTexture(&depthDesc);
}

void App::initDeviceAndRun() {
#if defined(__EMSCRIPTEN__)
    static const auto initDevice = [this]() {
        wgpu::DeviceDescriptor devDesc{};
        devDesc.SetUncapturedErrorCallback(
            [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
                std::cerr << "WebGPU error " << static_cast<int>(type)
                          << ": " << msg.data << std::endl;
            });
        adapter.RequestDevice(
            &devDesc,
            wgpu::CallbackMode::AllowSpontaneous,
            [this](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView msg) {
                if (status != wgpu::RequestDeviceStatus::Success) {
                    std::cerr << "RequestDevice failed: " << msg.data << std::endl;
                    exit(1);
                }
                device = std::move(d);
                configureSurface();
                createDepthTexture();
                onDeviceReady();
            });
    };
    wgpu::RequestAdapterOptions opts{ .compatibleSurface = surface };
    instance.RequestAdapter(
        &opts,
        wgpu::CallbackMode::AllowSpontaneous,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView msg) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cerr << "RequestAdapter failed: " << msg.data << std::endl;
                exit(1);
            }
            adapter = std::move(a);
            initDevice();
        });
#else
    wgpu::RequestAdapterOptions opts{ .compatibleSurface = surface };
    wgpu::Future f1 = instance.RequestAdapter(
        &opts,
        wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView msg) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cerr << "RequestAdapter failed: " << msg.data << std::endl;
                exit(1);
            }
            adapter = std::move(a);
        });
    instance.WaitAny(f1, UINT64_MAX);

    wgpu::DeviceDescriptor devDesc{};
    devDesc.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView msg) {
            std::cerr << "WebGPU error " << type << ": " << msg.data << std::endl;
        });
    wgpu::Future f2 = adapter.RequestDevice(
        &devDesc,
        wgpu::CallbackMode::WaitAnyOnly,
        [this](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView msg) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::cerr << "RequestDevice failed: " << msg.data << std::endl;
                exit(1);
            }
            device = std::move(d);
        });
    instance.WaitAny(f2, UINT64_MAX);
    configureSurface();
    createDepthTexture();
    onDeviceReady();
#endif
}

App::App(
    const int width, 
    const int height, 
    const char* title, 
    std::function<void()> onDeviceReady
)
    :wWidth(width),
    wHeight(height),
    wTitle(title),
    onDeviceReady(onDeviceReady)
{
    initGLFW();
    initWGPU();
    initSurface();
}

App::~App() {
    if (surface != nullptr) {
        surface.Unconfigure();
        surface = nullptr;
    }
    if (pipeline) pipeline = nullptr;
    if (surface) {
        surface.Unconfigure();
        surface = nullptr;
    }
    if (device) {
        device.Destroy();
        device = nullptr;
    }
    if (adapter) adapter = nullptr;
    if (instance) instance = nullptr;
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}