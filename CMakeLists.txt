cmake_minimum_required(VERSION 3.22) # CMake version check
project(app)                         # Create project "app"
set(CMAKE_CXX_STANDARD 20)           # Enable C++20 standard

# GLM
include(FetchContent)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glm)

# STB
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(stb)

add_executable(app 
  "src/main.cpp"
  "src/app.cpp"
  "src/camera.cpp"
)
target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${stb_SOURCE_DIR}"
)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)


if(EMSCRIPTEN)
  set_target_properties(app PROPERTIES SUFFIX ".html")
  # Use the emscripten-specific dawn target
  target_link_libraries(app PRIVATE emdawnwebgpu_cpp glm::glm)
  
  # Flags for Emscripten to handle the canvas and async nature of WebGPU
  target_link_options(app PRIVATE 
    "-sASYNCIFY=1" 
    "-sUSE_GLFW=3"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/src/shaders@/src/shaders"
    "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets"
  )
else()
  # Native build
  target_link_libraries(app PRIVATE webgpu_dawn webgpu_glfw glfw glm::glm)
endif()