#pragma once
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>

class App {
public:
    GLFWwindow*                 window;
    wgpu::Instance              instance;
    wgpu::Surface               surface;     
    wgpu::Device                device;
    wgpu::RenderPipeline        pipeline;
    wgpu::Texture               depthTexture;
    wgpu::TextureFormat         format;    
    const int                   wWidth;
    const int                   wHeight;
    const char*                 wTitle;
    const std::function<void()> onDeviceReady;
    App(const int widht, const int height, const char* title, std::function<void()> onDeviceReadyCallBack);
    ~App();
    void initDeviceAndRun();
private:
    wgpu::Adapter               adapter;
    void initGLFW();
    void initWGPU();
    void initSurface();
    void createDepthTexture();
    void configureSurface();
};