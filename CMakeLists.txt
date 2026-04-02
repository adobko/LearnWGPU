cmake_minimum_required(VERSION 3.22) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard

add_executable(app "src/main.cpp")

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

if(EMSCRIPTEN)
  set_target_properties(app PROPERTIES SUFFIX ".html")
  # Use the emscripten-specific dawn target
  target_link_libraries(app PRIVATE emdawnwebgpu_cpp) 
  
  # Flags for Emscripten to handle the canvas and async nature of WebGPU
  target_link_options(app PRIVATE "-sASYNCIFY=1" "-sUSE_GLFW=3")
else()
  # Native build
  target_link_libraries(app PRIVATE webgpu_dawn webgpu_glfw glfw)
endif()