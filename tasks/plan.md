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

## Follow-up Plan: Omen Runs and Explorer Echoes

### Scope and decisions

- Add one seed-linked atmosphere anomaly to each generated run: veiled moon, unreliable flashlight, distant bells, or stronger wind.
- Add discoverable survivor traces: some provide a small directional clue, some yield a battery, and some deliver only a short eerie note.
- Save the last completed run's sampled footsteps locally and project them as faint floor marks only where the next generated maze is walkable.
- Show current difficulty and anomaly on the title screen; allow Left/Right to change difficulty before starting. Keep Enter/Space and click as direct start actions.
- Add explicit next-run and return-to-title prompts to the escape screen. Do not add Echo Pulse or another start submenu.

### Ordered tasks

1. [x] Implement seed-linked anomaly selection and effects; show omen/difficulty on title; add title difficulty shortcut.
2. [x] Generate and render survivor traces, supplies, local exit-direction clues, and short discovery messages.
3. [x] Sample and persist the last completed route; show its faint, floor-safe footprints in the next maze.
4. [x] Clarify escape-screen actions, update README/graph, build the Release EXE, and push the finished changes.

### Acceptance and verification

- Every new maze has exactly one stable omen, with the matching atmosphere/audio/light effect applied during play.
- Title Left/Right changes and saves difficulty; Enter/Space/click still starts immediately.
- Traces are visible, non-blocking, discoverable, and their clue/battery/no-reward outcomes are distinct.
- Previous-run footprints load across launches, remain faint, and never render over walls.
- Escape-screen instructions match R-to-restart and Esc-to-title behavior.
- Release build succeeds; Graphify and README counts describe the shipped source.
