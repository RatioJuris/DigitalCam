# Phase 1 implementation scope

Phase 1 creates the shared foundation only. It does not install a system-level virtual camera or microphone.

## Included

- CMake project with C++20.
- Optional Qt 6 Studio application.
- FFmpeg discovery and decoder interfaces.
- Image and video metadata probing.
- Video frame decoding to BGRA.
- Audio decoding to 48 kHz stereo float.
- Playback state machine.
- Monotonic frame scheduler.
- Automatic looping controller.
- Aspect-ratio and pixel-format processing.
- Bounded frame queue.
- Configuration load/save.
- Logging facade.
- Camera and audio adapter contracts.
- In-process no-op adapters for desktop testing.
- Unit tests for non-OS components.

## Deferred

- Windows Media Foundation virtual camera.
- Windows virtual audio driver.
- Linux v4l2loopback and PipeWire adapters.
- macOS Camera Extension and Core Audio adapters.
- Installer/uninstaller.
- Playlist management.
- Multiple simultaneous virtual devices.
- Hardware-accelerated decoding.
- Android.

## Phase 1 exit criteria

- Project configures and builds with the documented dependencies.
- Unit tests pass.
- Studio starts without an OS virtual camera installed.
- A supported image or video can be loaded and decoded to a preview frame.
- Video end-of-stream restarts decoding when looping is enabled.
- Camera adapter lifecycle can be started and stopped independently from playback.
