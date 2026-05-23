#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <webgpu/webgpu_cpp.h>

class BindGroupBuilder {
public:
    std::string                             label;
    void addBuffer(
        uint32_t binding, 
        wgpu::ShaderStage visibility,
        wgpu::BufferBindingType type,
        bool hadDynamicOffset,
        uint64_t minBindingSize,
        const wgpu::Buffer& buffer, 
        uint64_t offset, 
        uint64_t size
    );
    void addTexture(
        uint32_t binding, 
        wgpu::ShaderStage visibility
    );
    void addSampler(
        uint32_t binding, 
        wgpu::ShaderStage visibility
    );
    wgpu::BindGroupLayout getLayout();
    wgpu::BindGroup build(wgpu::Device& device);
private:
    std::vector<wgpu::BindGroupLayoutEntry> layoutEntries;
    std::vector<wgpu::BindGroupEntry>       bgEntries;
    wgpu::BindGroupLayout                   layout;
    wgpu::BindGroupLayout buildLayout();
};