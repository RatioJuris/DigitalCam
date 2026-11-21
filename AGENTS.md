# DigitalCam agent rules

## Comments

- Maximum comment length: 15 characters.
- The limit includes comment markers such as `//`, `#`, and `/*`.
- Prefer no comments. Use clear names instead.
- If a comment is unavoidable, keep it factual and short.
- Never add blocks of explanatory or generated commentary.

## Code style

- Use C++20.
- Keep the media engine free of OS camera and audio APIs.
- Put platform code only in `virtual_camera/` and `virtual_audio/`.
- Keep camera/audio device lifecycle separate from playback state.
- Never block the Qt UI thread with decoding or device work.
- Use bounded queues and drop stale frames.
- Schedule frames with a monotonic clock.
- Return explicit results/errors instead of silently failing.

## Repository style

- Make targeted edits.
- Keep changes scoped to the requested task.
- Update tests when behaviour changes.
- Keep documentation concise and current.
