# Maze Escape

A first-person OpenGL maze game built directly on Win32/WGL. Each new run generates a connected maze with a circular starting chamber and gently irregular outer rings. A randomized depth-first search builds a spanning tree over polar cells, leaving exactly one route from the center to the single exit. The maze is converted to a fine grid so its walls look smoothly curved in the 3D view.

For a beginner-friendly file map, program flow, and explanation of the key functions, see the [project guide](docs/PROJECT_GUIDE.md).

## Code graph

The refreshed project graph maps 387 code and documentation symbols and 687 extracted relationships across 13 communities. Explore the [interactive graph](graphify-out/graph.html) or inspect its [graph data](graphify-out/graph.json).

![Maze Escape code graph](graphify-out/graph.svg)

## Build and run

Use a Windows C++ compiler and CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run `build\Release\MazeEscape.exe` with a Visual Studio generator, or `build\MazeEscape.exe` with a single-configuration generator. CMake copies the textures and flashlight model next to the executable.

## Play from the GitHub ZIP

Download and extract the repository ZIP, then double-click `MazeEscape.exe` in the extracted folder. Keep the `assets` folder beside the executable; it contains the textures and flashlight model. No build tools are needed.

## Controls

- **Enter / Space**: start a run from the title screen
- **P**: open Past Runs from the title; use **Left / Right** to page through completed runs
- **Left / Right on the title**: cycle Easy, Normal, and Hard
- **W / S**: move forward/back
- **A / D**: strafe
- **Shift**: sprint
- **Mouse**: look around
- **F**: toggle the flashlight (battery is conserved while it is off)
- **O**: reveal the route for one second (15-second cooldown on Normal; difficulty changes it)
- **R while playing**: use one collected battery cell to restore up to 50 charge
- **Esc**: pause; Esc again returns to the title screen
- **S**: open settings from the title screen or pause menu
- **Settings**: Up/Down selects; Left/Right changes; Enter toggles sound, head bob, or Back
- **Click**: start from the title screen or resume while paused
- **R after escaping**: start a new maze
- **Esc after escaping**: return to the title screen

The title screen starts a run directly and shows the current maze seed, personal best, difficulty, and a seed-linked omen. Settings include window resolution (960x600, 1280x800, 1600x900, or fullscreen), brightness, contrast, sound, difficulty, mouse sensitivity, and head bob. Choices are saved to `settings.ini` beside the executable. Starting charge, drain rate, and route-skill cooldown scale with difficulty. Collect glowing battery packs into inventory, then press **R** during a run to consume one and restore up to 50 charge. The brightness and contrast controls range from 50 to 150 percent.

The O skill runs A* on the compact room graph and briefly draws the shortest route through door centers. Each completed run reports how many times the route was revealed. The graph has 137 rooms, so pathfinding searches the actual maze structure instead of scanning all 313,600 collision cells. The flashlight casts filtered real-time shadows. Its beam may briefly stutter, with an electrical sound cue. The stone walls and forest floor use bundled 2K PBR maps for color, normal detail, roughness, and ambient occlusion, with scattered moss on the stone. The outer rings vary in shape on each map while the starting chamber remains circular. Selected wall ends have chipped silhouettes and small fallen stones; they are decorative and stay out of the route. A few narrow eye-level gaps let you glimpse adjacent corridors; they remain solid for movement. Corridors stay wide enough for comfortable movement. The timer stops when you escape. Footsteps, switch and pickup sounds, and a quiet stereo exit cue add spatial feedback. The exit cue fades with distance and is silent beyond 7 meters. The night sky has procedural stars, a moving moon, and drifting clouds. Clear sky brings strong blue moonlight; dense cloud cover dims the maze back to darkness.

The best time and its seed are saved locally in `settings.ini`. To replay a particular generated maze, run `MazeEscape.exe --seed 123456`; replace the number with the seed shown on the title screen or escape screen. Each new run still generates its own map unless a seed is supplied at launch.

Past Runs stores every completed run in `past_runs.csv` beside the executable. The in-game history lists the local completion timestamp, run time, route reveals used, omen, and maze seed, newest first. The file is created automatically after the first escape and stays local to that copy of the game.

## Omens and echoes

Each generated maze has one seed-linked omen. **Veiled Moon** keeps the clouds over the moon; **Failing Wick** makes the flashlight flicker more often; **Distant Bells** adds a quiet chime panned toward the exit; **Howling Wind** strengthens the procedural wind until it partly masks other sounds. The omen appears on the title and in the Past Runs list.

Five survivor traces are scattered through each maze. Scratched arrows give a brief direction toward the next part of the exit path, an abandoned cache adds a battery cell, and the remaining remnants share unsettling notes without a reward. Walking close reveals each trace automatically. After an escape, the game saves sampled footsteps to `last_run_trail.csv`; the next differently seeded maze shows faint footprints projected onto nearby walkable floor. They are visual echoes and do not affect walls, collisions, or pathfinding.

## Textures

The floor uses the 2K texture set from [Poly Haven Forest Ground 01](https://polyhaven.com/a/forrest_ground_01), and the walls use the 2K set from [Poly Haven Stone Wall 05](https://polyhaven.com/a/stone_wall_05). Both are CC0 assets and include diffuse, normal, roughness, and ambient-occlusion maps. The night sky is from [Poly Haven Qwantani Moon Noon (Pure Sky)](https://polyhaven.com/a/qwantani_moon_noon_puresky). The assets are bundled locally; the game does not need an internet connection to load them.

## Flashlight credit

"Old Flashlight" (https://skfb.ly/6zFGo) by Blender3D is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).
