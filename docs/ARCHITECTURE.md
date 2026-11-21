# DigitalCam architecture

## Invariants

1. The media engine is platform independent.
2. Camera and audio device lifecycle is separate from media playback state.
3. The UI never calls operating-system camera or audio APIs directly.
4. Frame queues are bounded and stale frames are dropped.
5. Frame scheduling uses a monotonic clock.
6. The virtual camera continues publishing frames while media is paused or replaced.
7. Media decoder failures do not terminate the device runtime.

## Component map

```text
app/                 Qt 6 studio UI
config/              Persistent application configuration
core/media/          Format probing, metadata, image/video decoding
core/playback/       State machine, scheduling, looping
core/video/          Scaling, aspect-ratio handling, pixel conversion, frame queues
core/audio/          Audio decoding, gain/mute, timestamp synchronisation
virtual_camera/      ICameraAdapter, manager, OS-specific implementations
virtual_audio/       IAudioAdapter, manager, OS-specific implementations
logging/             Shared logging facade
tests/               Unit, integration, and platform tests
```

## Device lifecycle

`startCamera()` and `stopCamera()` control the OS camera adapter. `loadMedia()`, `play()`, `pause()`, and `seek()` control media playback. They are independent except that playback publishes frames through an active camera adapter when one is available.

This lets applications such as OBS keep the camera registered while DigitalCam changes media or closes the Studio UI.

## Playback states

```text
Idle -> Ready -> Playing -> Paused -> Stopped
  ^       |         |         |         |
  |       v         v         v         v
  +---- Error <-----+---------+---------+
```

Transitions are validated by `PlaybackController`.

## Video pipeline

```text
Image/Video decoder
  -> frame normalisation
  -> resolution and aspect-ratio processing
  -> pixel-format conversion
  -> bounded timestamped frame queue
  -> camera adapter
```

## Audio pipeline

```text
Audio decoder
  -> resampling and channel conversion
  -> gain/mute processing
  -> timestamp alignment
  -> audio adapter
```

The virtual camera and virtual microphone are separate OS devices and are selected separately by consuming applications.
