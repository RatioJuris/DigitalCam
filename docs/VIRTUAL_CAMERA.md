# Windows virtual camera

DigitalCam registers a Windows 11 Media Foundation virtual camera named
`DigitalCam Virtual Camera`. Physical camera hardware is not required.

## Requirements

- Windows 11 build 22000 or newer.
- Administrator rights once for registration.
- The `DigitalCamVirtualCamera.dll` and `DigitalCamCameraInstaller.exe` files
  from the Windows release archive.

## Install

Open an elevated terminal in the folder that contains the Windows build and run:

```powershell
.\DigitalCamCameraInstaller.exe --install
```

The installer copies the COM media source to `C:\Program Files\DigitalCam`,
grants the Frame Server services access, and registers the CLSID under
`HKLM`.

## Use

1. Start `DigitalCamStudio.exe`.
2. Select an image or video.
3. Choose the output resolution and frame rate.
4. Select `Start Camera`.
5. In the Camera app or a browser, select `DigitalCam Virtual Camera`.

The camera exists while Studio is running. Stopping the camera removes the
device. The frame bridge is stored in `C:\ProgramData\DigitalCam` and is
accessible to the Frame Server services.

## CLI and headless use

`DigitalCamStudio.exe` and `DigitalCamCli.exe` accept the same options:

```powershell
.\DigitalCamCli.exe --media .\sample.png --width 1280 --height 720 `
  --fps 30 --start-camera --duration 60
```

`DigitalCamStudio.exe --headless` runs the same flow without a window.
`DigitalCamCli.exe` is a console executable, so logs are written to both the
terminal and `logs\digitalcam.log`. The media path may be supplied directly or
with `--media`. Long options also accept `--name=value`.

| Option | Purpose |
| --- | --- |
| `--media <path>` | Image or video to publish. |
| `--width <pixels>` | Output width, 1-1920. Default 1280. |
| `--height <pixels>` | Output height, 1-1080. Default 720. |
| `--fps <rate>` | Output frame rate. Default 30. |
| `--start-camera` | Register the OS virtual camera at startup. |
| `--headless` | Run Studio without a GUI window. |
| `--duration <seconds>` | Stop and remove the camera after the delay. |
| `--log-dir <path>` | Log directory. Default `logs`. |
| `--verbose` | Include debug logs. |
| `--help` | Print usage. |
| `--version` | Print the version. |

Exit codes: `0` success, `1` media or camera failure, `2` invalid arguments.

## Uninstall

```powershell
.\DigitalCamCameraInstaller.exe --uninstall
```

## Troubleshooting

If Studio reports that the camera is not installed, run the installer as
Administrator. If a browser does not list the camera, close the browser and
reopen it after Studio has started the camera.

Logs are flushed after every message. `logs\digitalcam.log` records media
loading, bridge setup, virtual camera creation, and camera startup, so it
identifies the last successful step before a failure.

The COM media source is adapted from the MIT-licensed VCamSample reference.
See `third_party/VCamSample-LICENSE.txt`.
