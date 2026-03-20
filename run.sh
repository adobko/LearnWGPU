#!/bin/bash

cmake --build build -j4
build/app

while ! hyprctl clients | grep -q "class: WebGPU"; do
    sleep 0.01
done

hyprctl dispatch togglefloating class:"WebGPU" > /dev/null
hyprctl dispatch resizewindowpixel exact 800 600,class:"WebGPU" > /dev/null
hyprctl dispatch centerwindow class:"WebGPU" > /dev/null