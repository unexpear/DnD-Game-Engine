# Building the D&D Game Engine

## Prerequisites

| Tool | Minimum Version | Purpose |
|------|----------------|---------|
| CMake | 3.20 | Build system configuration |
| C++ Compiler | C++17 support | Visual Studio 2022, GCC 9+, Clang 9+ |
| Git | Any | Cloning third-party dependencies |

## Step 1: Clone Third-Party Dependencies

The engine depends on three libraries placed in `third_party/`:

### GLFW (Window & Input)
```
cd third_party
git clone https://github.com/glfw/glfw.git glfw
```

### GLAD (OpenGL Loader)
1. Go to https://glad.dav1d.de/
2. Set options:
   - **Language**: C/C++
   - **Specification**: gl
   - **API**: gl, Version: 4.6
   - **Profile**: Core
   - **Options**: Generate a loader (checked)
3. Click "Generate", then download the zip
4. Extract to `third_party/glad/` — should contain `include/` and `src/`

### stb_image (Image Loading)
1. Download `stb_image.h` from https://github.com/nothings/stb/blob/master/stb_image.h
2. Place in `third_party/stb/stb_image.h`

```
third_party/
├── glfw/          (cloned repo)
├── glad/
│   ├── include/   (generated headers)
│   └── src/       (generated source)
└── stb/
    └── stb_image.h
```

## Step 2: Configure CMake

From the project root:

```
cmake -B build -S .
```

For Visual Studio:
```
cmake -B build -S . -G "Visual Studio 17 2022"
```

For a specific build type (Debug/Release):
```
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
```

## Step 3: Build

```
cmake --build build --config Debug
```

Or for Release:
```
cmake --build build --config Release
```

## Step 4: Run

```
build/DnDEngine.exe
```

Or with command-line flags:
```
build/DnDEngine.exe --debug --width=1920 --height=1080
```

## Troubleshooting

### "Could not find OpenGL"
- OpenGL comes with Windows. If missing, update graphics drivers.
- On Linux: `sudo apt-get install libgl1-mesa-dev`

### "GLFW not found"
- Verify `third_party/glfw/CMakeLists.txt` exists
- The glfw directory must contain the full cloned repo

### "GLAD not found"
- Verify `third_party/glad/src/glad.c` exists
- Verify `third_party/glad/include/glad/glad.h` exists

### Linker errors about missing symbols
- Run `cmake -B build -S .` again to regenerate build files
- Check that all `.cpp` files are in `src/` directories
