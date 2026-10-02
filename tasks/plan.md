# Implementation Plan: Maze Escape Expansion

## Task List

1. [x] Generate a randomized, mostly circular perfect maze from polar cells; render/collide against its rasterized walls; route skill uses A* to the designated exit.
2. [x] Deepen the night lighting and add a settings screen for window resolution, brightness, contrast, and sound, with saved preferences.
3. [x] Convert and optimize the supplied Chisa and flashlight models; add first-person flashlight and third-person follow-camera modes; preserve both CC-BY credits.
4. [ ] Build and manually inspect the game, run `/graphify`, update the packaged executable/assets and README, commit, and push.

## Decisions and Limits

- Keep the existing Win32/WGL application and current collision/render path where possible.
- Use a randomized spanning tree for unique start-to-exit paths. A* targets the fixed exit cell.
- The supplied FBX has no animation actions; if that remains true after conversion, the character will be a static posed model with camera-follow movement.
- User requested skipping unavailable items rather than blocking the rest.

## Verification

- Build after each major slice.
- Inspect the generated maze, route, settings, and camera modes in the running game when the renderer supports them.
- Verify the packaged EXE and required assets are together before pushing.
