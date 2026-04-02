#!/bin/bash

if [ "$1" != "--debug" ]; then
    cmake -B build
fi

cmake --build build -j4
build/app &

# This is to make the window float and be the correct size on Hyprland
if [ -n "$HYPRLAND_INSTANCE_SIGNATURE" ]; then
    while ! hyprctl clients | grep "class: WebGPU" > /dev/null; do
        sleep 0.01
    done

    hyprctl dispatch togglefloating class:"WebGPU" > /dev/null
    hyprctl dispatch resizewindowpixel exact 800 600,class:"WebGPU" > /dev/null
    hyprctl dispatch centerwindow class:"WebGPU" > /dev/null
fi