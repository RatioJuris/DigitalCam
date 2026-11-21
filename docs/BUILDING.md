# Building DigitalCam

## GitHub Actions

Pushes and pull requests to `main` run the Windows x64 and Ubuntu x64 build and test jobs defined in `.github/workflows/build.yml`.

## Local prerequisites

- CMake 3.21 or newer.
- C++20 compiler.
- Qt 6: Core, Gui, Widgets.
- FFmpeg development libraries: avformat, avcodec, avutil, swscale, swresample.
- GoogleTest for unit tests.
- spdlog is optional. The logger falls back to standard output when unavailable.

The CI configuration uses dynamic FFmpeg libraries. Static FFmpeg builds also require their transitive system libraries to be available to the linker.

On Windows, vcpkg is the simplest way to supply dependencies.

```powershell
vcpkg install qtbase:x64-windows ffmpeg:x64-windows spdlog:x64-windows gtest:x64-windows
```

## Configure

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
```

If FFmpeg is installed outside the default search paths:

```powershell
cmake -S . -B build -DFFMPEG_ROOT="C:\Path\To\ffmpeg"
```

## Build

```powershell
cmake --build build --config Release
```

## Test

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Run

```powershell
.\build\app\Release\DigitalCamStudio.exe
```

The exact executable path depends on the CMake generator and Qt configuration.
