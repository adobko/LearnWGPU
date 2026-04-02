#!/bin/bash

if [ "$1" != "--debug" ]; then
    cmake -B build
fi

cmake --build build-web -j4
npx serve ./build-web &

# This is to setup a browser session compatible with WebGPU on Hyprland
if [ -n "$HYPRLAND_INSTANCE_SIGNATURE" ]; then
    chromium \
        --ozone-platform=x11 \
        --enable-unsafe-swiftshader \
        --enable-unsafe-webgpu \
        --use-angle=swiftshader \
        --enable-features=ReduceOpsTaskSplitting,Vulkan,VulkanFromANGLE,DefaultANGLEVulkan \
        http://localhost:3000/app
    pkill chromium
    pkill -f npx
fi