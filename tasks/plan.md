# Implementation Plan: Maze Escape Expansion

## Task List

1. [x] Generate a randomized, mostly circular perfect maze from polar cells; render/collide against its rasterized walls; route skill uses A* to the designated exit.
2. [x] Deepen the night lighting and add a settings screen for window resolution, brightness, contrast, and sound, with saved preferences.
3. [x] Keep the first-person flashlight; remove the character model and third-person camera.
4. [x] Build and smoke-check startup, run `/graphify`, update the packaged executable/assets and README, commit, and push.

## Decisions and Limits

- Keep the existing Win32/WGL application and current collision/render path where possible.
- Use a randomized spanning tree for unique start-to-exit paths. A* targets the fixed exit cell.
- User requested skipping unavailable items rather than blocking the rest.

## Verification

- The Release build compiled with first-person flashlight rendering.
- Confirm the packaged EXE and flashlight textures are beside the runtime assets.
- Graphify output was refreshed and pushed with the source changes.
