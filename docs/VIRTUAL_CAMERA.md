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

## Uninstall

```powershell
.\DigitalCamCameraInstaller.exe --uninstall
```

## Troubleshooting

If Studio reports that the camera is not installed, run the installer as
Administrator. If a browser does not list the camera, close the browser and
reopen it after Studio has started the camera.

The COM media source is adapted from the MIT-licensed VCamSample reference.
See `third_party/VCamSample-LICENSE.txt`.
