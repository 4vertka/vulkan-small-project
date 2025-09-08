# Vulkan 2D Engine

A small 2D engine built with [Vulkan](https://www.vulkan.org/).  

---

## Features
- [x] Basic Vulkan setup (instance, devices, swapchain, pipeline)
- [x] 2D sprite rendering
- [x] Camera movement
- [x] Simple UI using ImGui
- [ ] Texture loading improvements
- [ ] Basic animation
- [x] Entity system

---

## Screenshot

![Screenshot](https://github.com/4vertka/vulkan-small-project/raw/main/Screenshot%20from%202025-06-25%2007-56-09.png)

---

## Build Instructions

### Requirements
- C++20 compiler
- [Vulkan SDK]
- [GLFW]
  
### Build (Linux / Arch example)
```bash
git clone https://github.com/4vertka/vulkan-small-project.git
cd vulkan-small-project
mkdir build && cd build
cmake ..
make
./MyVulkanApp
