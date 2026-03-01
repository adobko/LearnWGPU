cmake --build build
./build/app

# while ! hyprctl clients | grep -q "class: opengl_test_app"; do
#     sleep 0.01
# done

# hyprctl dispatch togglefloating class:"opengl_test_app" > /dev/null
# hyprctl dispatch resizewindowpixel exact 800 600,class:"opengl_test_app" > /dev/null
# hyprctl dispatch centerwindow class:"opengl_test_app" > /dev/null