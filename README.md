# N-Body GPU

## Table of contents

- [Description](#description)
- [Controls](#controls)
- [Installation](#installation)
  - [Requirements](#requirements)
  - [Clone the repository](#clone-the-repository)
  - [Build the project](#build-the-project)
  - [Run the program](#run-the-program)
- [Libraries](#libraries)
- [License](#license)

## Description

N-Body simulation that runs on the GPU with OpenGL.

## Controls

- Camera is centered on (0, 0, 0). Use middle mouse button to move around the origin and mouse wheel to zoom in/out.
- ESC to close window

## Installation

### Requirements

- OpenGL
- CMake >= 3.30
- C++20
- C++ compiler (gcc, g++, clang)
- Python >= 3.7

### Jinja

For GLM you need to install Jinja:

```sh
python -m pip install jinja2
```

### Clone the repository

```sh
git clone https://github.com/Corentin-Mzr/nbody-gpu.git
```

### Build the project

From the root folder, execute the following commands:

```sh
cmake -B build
cmake --build build
```

### Run the program

To run the program, launch it from the build/bin folder:

```sh
cd build/bin
./NBody-GPU
```

## Libraries

- [**GLFW**](https://github.com/glfw/glfw)
- [**GLAD**](https://github.com/g-truc/glm)
- [**GLM**](https://github.com/Dav1dde/glad)

## License

This program is under the [**MIT License**](LICENSE.md).
