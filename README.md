# ToyRenderer

A C++17/OpenGL 3.3 real-time renderer for a PMX interior scene and an OBJ
character model. The project is organized as a small multi-pass renderer and
includes runtime controls through Dear ImGui.

## Features

- PMX and OBJ model loading through Assimp
- Classic Blinn-Phong and Cook-Torrance PBR material paths
- MTL material support for `Kd`, `Ks`, `Ns`, `Ke`, `map_Kd`, and `map_Ke`
- HDR rendering, emissive materials, exposure tone mapping, and gamma correction
- Omnidirectional point-light shadow mapping with a depth cubemap
- Hardware shadow comparison, seamless cubemap sampling, and Vogel-disk PCF
- View-space SSAO with a hemisphere kernel, noise rotation, and blur pass
- Runtime-selectable 2x, 4x, and 8x MSAA with HDR framebuffer resolve
- Dear ImGui controls for lighting, shadows, SSAO, MSAA, camera, and scene objects

## Clone

The third-party dependencies are Git submodules:

```powershell
git clone --recurse-submodules https://github.com/Kaslana-423/ToyRenderer.git
cd ToyRenderer
```

If the repository was cloned without `--recurse-submodules`, run:

```powershell
git submodule update --init --recursive
```

## Build on Windows

Requirements:

- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.20 or newer

```powershell
cmake --preset vs2022-x64
cmake --build --preset debug
.\build\Debug\MyRenderer.exe
```

The post-build step copies the shaders and the active scene assets beside the
executable so that it can be launched directly from `build/Debug`.

## Controls

- `W`, `A`, `S`, `D`: move the camera
- Mouse: look around
- Mouse wheel: zoom
- `Tab`: switch between camera control and UI parameter editing
- `Esc`: quit

## Project layout

```text
include/            Renderer, scene, model, mesh, shader, and camera headers
src/                Renderer passes, scene management, UI, and application entry
shaders/            Lighting, shadow, SSAO, and HDR GLSL shaders
res/model/          Active PMX room and OBJ character assets
external/           Git submodules for GLFW, GLM, stb, Assimp, and Dear ImGui
third_party/glad/   Pre-generated OpenGL 3.3 Core loader
```

## Asset note

The scene and character assets are used for learning and rendering
demonstration. Their original copyrights and redistribution terms remain with
their respective creators.
