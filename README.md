<div style="text-align: center;">

# ShowcaseRenderer

### A modern, high-performance Vulkan rendering engine built from scratch in C++20.

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Vulkan](https://img.shields.io/badge/API-Vulkan-red.svg)](https://www.vulkan.org/)
[![CMake](https://img.shields.io/badge/Build-CMake-green.svg)](https://cmake.org/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

</div>

---

## About

I recently graduated with a degree in Computer Science, and a specialization in Creative Computational Media.
That was my university's term for the Computer Graphics pathway which partnered with our Academy of Creative Media.
As a Graphics Programming hobbyist, I was lucky to be among the first 4 graduates in this track.
This rendering engine exists to showcase what I've learned and what I can do.

> **Objective:** Implement and optimize modern rendering techniques using the Vulkan API.
> I am particularly interested in Path Tracing, and exploring academic research papers on the topic.

---

## Tech Stack & Architecture

* **Graphics API:** Vulkan SDK
* **Language Standard:** C++20
* **Windowing & Input:** SDL3 & ImGui
* **Vector Math:** GLM (OpenGL Mathematics)
* **GPU Memory Management:** VMA (Vulkan Memory Allocator)


### **Current Development Focus:**

* Vulkan Backend Core
* SPIR-V Support
* Compute Shader Support

---

## Project Roadmap

<details>
<summary>Click to expand full engine roadmap</summary>

<table>
  <tr>
    <tc>
      <td style="text-align: center;">
        <h3>Engine Core</h3>
      </td>
    </tc>
    <tc>
      <td>

* [ ] Vulkan Backend
* [ ] Asset Memory Management
* [ ] Entity-Component-System
* [ ] Post-Process Shading Pipeline
* [ ] Multithreading
* [ ] SIMD for CPU Vector Math
* [ ] Physics Engine

</td>
    </tc>
  </tr>
  <tr>
    <tc>
      <td style="text-align: center;">

### **Types**

</td>
    </tc>
    <tc>
     <td>

* [ ] PBR Materials
* [ ] Textures
* [ ] Mesh Loading
* [ ] Procedural Meshes
* [ ] Implicit Geometry

</td>
    </tc>
  </tr>
  <tr>
    <tc>
      <td style="text-align: center;">

### **Shaders**

</td>
    </tc>
    <tc>
      <td>

* [ ] Spir-V Support
* [ ] GLSL Support
* [ ] HLSL Support
* [ ] Runtime compilation
* [ ] Auto-Recompilation
* [ ] C Preprocessor

</td>
    </tc>
  </tr>
  <tr>
    <tc>
      <td style="text-align: center;">

### **Rasterization**

</td>
    </tc>
    <tc>
      <td>

* [ ] Hardware Rasterization Pipeline
* [ ] Blinn-Phong Shading

</td>
    </tc>
  </tr>
  <tr>
    <tc>
      <td style="text-align: center;">

### **Ray Tracing**

</td>
    </tc>
    <tc>
      <td>

* [ ] Monte Carlo Path Tracing
* [ ] Temporal Anti-Aliasing
* [ ] Next Event Estimation
* [ ] Multiple Importance Sampling
* [ ] Metropolis Light Transport
* [ ] Wavefront Path Tracing
* [ ] Denoiser
* [ ] Offline GPU Rendering
* [ ] Hardware Ray Tracing Pipeline

</td>
    </tc>
  </tr>
</table>

</details>

---

## Build Instructions

Run this project locally!

### Prerequisites

To build this project, first grab these dependencies:
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).
* [CMake](https://cmake.org/).
* A C++ Compiler (GCC, Clang, or MSVC)

### Setup

1. Clone this repository & submodules with the following command:
```bash
git clone --recurse-submodules https://github.com/kylebueche/ShowcaseRenderer.git
```
2. Build the Executable using CMake:
```bash
cd ShowcaseRenderer
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
3. Run the executable!
