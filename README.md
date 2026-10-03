# Maze Escape

A first-person OpenGL maze game built directly on Win32/WGL. Each new run generates a connected maze with a circular starting chamber and gently irregular outer rings. A randomized depth-first search builds a spanning tree over polar cells, leaving exactly one route from the center to the single exit. The maze is converted to a fine grid so its walls look smoothly curved in the 3D view.

## Code graph

The refreshed project graph maps 329 code and documentation symbols and 564 extracted relationships across 13 communities. Explore the [interactive graph](graphify-out/graph.html) or inspect its [graph data](graphify-out/graph.json).

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

- **Enter / Space**: open the Run Menu from the title; start from the Run Menu
- **W / S**: move forward/back
- **A / D**: strafe
- **Shift**: sprint
- **Mouse**: look around
- **F**: toggle the flashlight (battery is conserved while it is off)
- **O**: reveal the route for one second (15-second cooldown on Normal; difficulty changes it)
- **R while playing**: use one collected battery cell to restore up to 50 charge
- **Esc**: pause; Esc again returns to the title screen
- **Run Menu Left/Right**: change difficulty; **Esc** returns to title
- **S**: open settings from the title screen, Run Menu, or pause menu
- **Settings**: Up/Down selects; Left/Right changes; Enter toggles sound, head bob, or Back
- **Click**: open Run Menu, start from Run Menu, or resume while paused
- **R after escaping**: start a new maze

The title screen leads to a separate Run Menu for the maze seed, difficulty, best record, and replay instructions. Settings include window resolution (960x600, 1280x800, 1600x900, or fullscreen), brightness, contrast, sound, difficulty, mouse sensitivity, and head bob. Choices are saved to `settings.ini` beside the executable. Starting charge, drain rate, and route-skill cooldown scale with difficulty. Collect glowing battery packs into inventory, then press **R** during a run to consume one and restore up to 50 charge. The brightness and contrast controls range from 50 to 150 percent.

The O skill runs A* on the walkable grid and briefly draws the shortest route to the exit. Each completed run reports how many times the route was revealed. The game maze is a spanning tree, so there is only one corridor route to solve; A* efficiently finds that route from the player's current position. The flashlight casts filtered real-time shadows. Its beam may briefly stutter, with an electrical sound cue. The stone walls and forest floor use bundled 2K PBR maps for color, normal detail, roughness, and ambient occlusion, with scattered moss on the stone. The outer rings vary in shape on each map while the starting chamber remains circular. Selected wall ends have chipped silhouettes and small fallen stones; they are decorative and stay out of the route. A few narrow eye-level gaps let you glimpse adjacent corridors; they remain solid for movement. Corridors stay wide enough for comfortable movement. The timer stops when you escape. Footsteps, switch and pickup sounds, and a quiet stereo exit cue add spatial feedback. The exit cue fades with distance and is silent beyond 7 meters. The night sky has procedural stars, a moving moon, and drifting clouds. Clear sky brings strong blue moonlight; dense cloud cover dims the maze back to darkness.

The Run Menu shows the current maze seed and your best time. The best time and its seed are saved locally in `settings.ini`. To replay a particular generated maze, run `MazeEscape.exe --seed 123456`; replace the number with the seed shown in the Run Menu or escape screen. Each new run still generates its own map unless a seed is supplied at launch.

## Textures

The floor uses the 2K texture set from [Poly Haven Forest Ground 01](https://polyhaven.com/a/forrest_ground_01), and the walls use the 2K set from [Poly Haven Stone Wall 05](https://polyhaven.com/a/stone_wall_05). Both are CC0 assets and include diffuse, normal, roughness, and ambient-occlusion maps. The night sky is from [Poly Haven Qwantani Moon Noon (Pure Sky)](https://polyhaven.com/a/qwantani_moon_noon_puresky). The assets are bundled locally; the game does not need an internet connection to load them.

## Flashlight credit

"Old Flashlight" (https://skfb.ly/6zFGo) by Blender3D is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).
