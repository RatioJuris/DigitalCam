# DigitalCam

DigitalCam is a cross-platform virtual camera application that turns a local image or video file into a live camera feed recognised by the host operating system.

Current repository status: **Phase 1 scaffold**.

## Phase 1 scope

The current codebase establishes the foundation for:

- C++20 and CMake project structure.
- Qt 6 desktop studio shell.
- FFmpeg-backed image and video decoding interfaces.
- Platform-independent media, playback, frame, and audio components.
- Explicit playback state machine and monotonic frame scheduling.
- Camera/audio adapter abstraction with non-OS stub implementations.
- Bounded queues, logging, configuration, and unit tests.

Native OS virtual camera/audio implementations are deliberately deferred to later phases:

- Windows: Media Foundation virtual camera and virtual audio device.
- Linux: v4l2loopback and PipeWire.
- macOS: Core Media I/O Camera Extension and Core Audio.

## Architecture

```text
DigitalCam Studio -> Media Engine -> Video/Audio Adapters -> Operating System -> Other Applications
```

The media engine must not call operating-system camera or audio APIs directly. Platform-specific code lives behind `ICameraAdapter` and `IAudioAdapter`.

## Continuous integration

GitHub Actions builds and tests the project on Windows and Ubuntu for pushes and pull requests to `main`.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Phase 1 implementation scope](docs/PHASE_1.md)
- [Build instructions](docs/BUILDING.md)
- [Licensing notes](docs/LICENSING.md)

## Repository

https://github.com/RatioJuris/DigitalCam
