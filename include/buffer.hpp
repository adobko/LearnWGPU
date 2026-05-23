#pragma once
#include <cstring>
#include <vector>
#include <webgpu/webgpu_cpp.h>

wgpu::Buffer createBuffer(wgpu::Device& device, wgpu::BufferUsage usage, const void* data, size_t size) {
    wgpu::BufferDescriptor desc{
        .usage            = usage,
        .size             = (size + (16-1)) & ~(16-1), // Round up to 16-byte alligment
        .mappedAtCreation = true,
    };
    wgpu::Buffer buffer = device.CreateBuffer(&desc);
    memcpy(buffer.GetMappedRange(), data, size);
    buffer.Unmap();
    return buffer;
}

template<typename T>
wgpu::Buffer createUniformBuffer(wgpu::Device& device, T& data) {
    return createBuffer(
        device, 
        wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, 
        &data, 
        sizeof(data)
    );
}

template<typename T>
wgpu::Buffer createStorageBuffer(wgpu::Device& device, std::vector<T>& data) {
    return createBuffer(
        device, 
        wgpu::BufferUsage::Storage | wgpu::BUfferUsage::CopyDst,
        data.data(),
        data.size() * sizeof(T)
    );
}