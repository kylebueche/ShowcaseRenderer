# ShowcaseRenderer

A modern, high-performance Vulkan rendering engine built from scratch in C++20.

The goal of this project is to implement as many path tracing techniques as I can.

## Project Status

This project is actively under development.

Desired Features:

- [ ] Mesh Support
- [ ] Implicits Support
- [ ] Blinn-Phong Shading
- [ ] Basic Materials
- [ ] PBR Materials
- [ ] Monte Carlo Path Tracing
- [ ] Hardware Ray Tracing
- [ ] Temporal Anti-Aliasing
- [ ] Next Event Estimation
- [ ] Multiple Importance Sampling
- [ ] Metropolis Light Transport
- [ ] Wavefront Path Tracing

## Tech Stack & Architecture
* **Graphics API:** Vulkan SDK
* **Language Standard:** C++20
* **Windowing & Input:** SDL3 & ImGui
* **Vector Math:** GLM
* **GPU Memory Management:** VulkanMemoryAllocator

## Build Instructions
You must first install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home), and then grab [CMake](https://cmake.org/). After that, the project can be compiled from any IDE that supports CMake.