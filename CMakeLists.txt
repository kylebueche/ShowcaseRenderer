# ===================================
# Project Settings
# ===================================
cmake_minimum_required(VERSION 3.22)
project(ShowcaseRenderer CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

# ===========================================================
# Begin ImGui Project
# ImGui Doesn't Have it's own CMakeLists.txt
# ===========================================================
project(imgui VERSION 1.90)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

# ImGui Core
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
)

# Imgui Backend Implementation
list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Make it a Static Library
add_library(imgui STATIC ${IMGUI_SOURCES})

# Expose .h files Upstream
target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)
# ===========================================================
# End ImGui Project
# ===========================================================

# Add Header Only Libraries
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# Find Local Vulkan Installation
find_package(Vulkan REQUIRED COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
# ============================================================
# Compile Shaders
# ============================================================
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

file(GLOB_RECURSE GLSL_SHADERS
        # Generic/Nondescript:
        ${SHADER_SOURCE_DIR}/*.glsl
        # Common:
        ${SHADER_SOURCE_DIR}/*.vert
        ${SHADER_SOURCE_DIR}/*.frag
        ${SHADER_SOURCE_DIR}/*.geom
        ${SHADER_SOURCE_DIR}/*.comp
        ${SHADER_SOURCE_DIR}/*.tesc
        ${SHADER_SOURCE_DIR}/*.tese
        # Alternate Common:
        ${SHADER_SOURCE_DIR}/*.vs
        ${SHADER_SOURCE_DIR}/*.fs
        ${SHADER_SOURCE_DIR}/*.gs
        # Mesh Shader Pipelines:
        ${SHADER_SOURCE_DIR}/*.mesh
        ${SHADER_SOURCE_DIR}/*.task
        # Hardware Ray Tracing Pipelines:
        ${SHADER_SOURCE_DIR}/*.rgen
        ${SHADER_SOURCE_DIR}/*.rint
        ${SHADER_SOURCE_DIR}/*.rahit
        ${SHADER_SOURCE_DIR}/*.rchit
        ${SHADER_SOURCE_DIR}/*.rmiss
        ${SHADER_SOURCE_DIR}/*.rcall
)

set(SPV_SHADERS "")

foreach(source IN LISTS GLSL_SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  set(OUTPUT_SPV ${SHADER_BINARY_DIR}/${FILENAME}.spv)

  add_custom_command(
          OUTPUT ${OUTPUT_SPV}
          # 1. Create the output directory safely right before compiling
          COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
          # 2. Compile the shader
          COMMAND
          ${glslc_executable}
          -o ${OUTPUT_SPV}
          ${source}
          # Only depend on the actual source file, NOT the directory
          DEPENDS ${source}
          COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${OUTPUT_SPV})
endforeach()

# This target drives the generation of the individual SPV files
add_custom_target(glsl_shaders ALL DEPENDS ${SPV_SHADERS})


# SPIRV-Reflect Build Settings
# ===============================================================================
set(SPIRV_REFLECT_STATIC_LIB     ON  CACHE BOOL "" FORCE) # Get the library
set(SPIRV_REFLECT_EXECUTABLE     OFF CACHE BOOL "" FORCE) # Disable CLI tools
set(SPIRV_REFLECT_INSTALL        OFF CACHE BOOL "" FORCE) # Dont pollute install paths
set(SPIRV_REFLECT_BUILD_TESTS    OFF CACHE BOOL "" FORCE) # Disable Tests

# Add Libraries
# ===================================================================
add_subdirectory(third_party/SDL EXCLUDE_FROM_ALL)
add_subdirectory(third_party/VulkanMemoryAllocator EXCLUDE_FROM_ALL)
add_subdirectory(third_party/glm EXCLUDE_FROM_ALL)
add_subdirectory(third_party/tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(third_party/vk-bootstrap EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spirv-reflect EXCLUDE_FROM_ALL)
add_subdirectory(third_party/fmt EXCLUDE_FROM_ALL)

# Imgui Needs Access to these Libraries
# =============================================================
target_link_libraries(imgui PRIVATE Vulkan::Vulkan SDL3::SDL3)

# Gather Source Files
# ===============================================
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")
add_executable(ShowcaseRenderer ${SOURCE_FILES})

# What is Tracy? VKGuide is kind of incomplete..
# target_compile_definitions(ShowcaseRenderer PUBLIC TRACY_ENABLE)
target_compile_definitions(ShowcaseRenderer PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)

target_include_directories(ShowcaseRenderer PUBLIC src)
target_precompile_headers(ShowcaseRenderer PUBLIC "<unordered_map>" "<vector>" "<iostream>" "<fstream>" "<string>" )

# Link all the dependencies
target_link_libraries(ShowcaseRenderer PRIVATE
        Vulkan::Vulkan
        SDL3::SDL3
        vk-bootstrap
        VulkanMemoryAllocator
        glm
        tinyobjloader
        imgui
        stb_image
        spirv-reflect-static
        fmt::fmt)

add_dependencies(ShowcaseRenderer glsl_shaders)