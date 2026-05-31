# Project Settings
# ==================================
cmake_minimum_required(VERSION 3.22)
project(ShowcaseRenderer CXX)
set(CMAKE_CXX_STANDARD 20)

# Add Header Only Libraries
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# Find Local Vulkan Installation
find_package(Vulkan REQUIRED)

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
add_subdirectory(third_party/imgui EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spirv-reflect EXCLUDE_FROM_ALL)

# Imgui Needs Access to these Libraries
# =============================================================
target_link_libraries(imgui PRIVATE Vulkan::Vulkan SDL3::SDL3)

# Gather Source Files
# ===============================================
file(GLOB SOURCE_FILES "src/*.cpp" "src/*.h")
add_executable(ShowcaseRenderer ${SOURCE_FILES})

# What is Tracy? VKGuide is kind of incomplete..
# target_compile_definitions(ShowcaseRenderer PUBLIC TRACY_ENABLE)
target_compile_definitions(ShowcaseRenderer PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)

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
        spirv-reflect-static)
