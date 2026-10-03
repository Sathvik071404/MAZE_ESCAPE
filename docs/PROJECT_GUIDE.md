# Maze Escape Project Guide

This guide explains the files that make up the game and the main jobs performed by its code. The game is a native Windows application: C++ drives the maze and menus, OpenGL draws the world, and the files in `assets/` provide its models, textures, and sounds.

## Project files

| File or folder | What it is for |
| --- | --- |
| `.gitignore` | Keeps build output, debug files, and each player's local saves out of Git. |
| `CMakeLists.txt` | Tells CMake how to compile the Windows game and copy its runtime assets beside the executable. |
| `src/main.cpp` | Contains the game state, maze generation, movement, collision, saves, Win32 input, audio, OpenGL renderer, shaders, and menus. Explanatory comments sit next to the main functions and the more involved calculations. |
| `MazeEscape.exe` | Packaged Windows build. Players can run it directly after extracting the repository ZIP, as long as `assets/` stays beside it. |
| `README.md` | Player instructions, controls, build steps, game features, and attribution for downloaded assets. |
| `LICENSE` | License information for the project. Individual third-party assets have their own credits in the README. |
| `docs/PROJECT_GUIDE.md` | This beginner-friendly explanation of the repository and code flow. |
| `graphify-out/graph.html` | Interactive generated map of code relationships. |
| `graphify-out/graph.json` | Machine-readable data used by the generated code graph. |
| `graphify-out/graph.svg` | Static version of the generated code graph shown in the README. |
| `assets/models/flashlight/Flashlight.obj` | Flashlight shape: vertices and faces used to draw the model. |
| `assets/models/flashlight/Flashlight.mtl` | Material file that connects the OBJ flashlight to its image texture. |
| `assets/models/flashlight/Diffuse.png` | Flashlight surface color. |
| `assets/sounds/*.wav` | Short effects for footsteps, the menu, route skill, flashlight, batteries, and escaping. |
| `assets/textures/night_sky.ppm` | Base sky image sampled by the sky shader. |
| `assets/textures/pbr/forest_ground_*` | Ground color, normal detail, roughness, and ambient-occlusion maps. |
| `assets/textures/pbr/stone_wall_*` | Wall color, normal detail, roughness, and ambient-occlusion maps. |

Old maze image/map files and an unused HDR sky were removed because the game generates its map and loads the PPM sky instead. The old `tasks/` checklist was also removed; its entries described already-finished work rather than player or developer instructions.

## How the program runs

1. `WinMain` loads the seed argument and local saves, generates the first maze, creates the window and OpenGL context, then initializes audio and rendering.
2. Each frame reads Windows messages, advances `updateAtmosphere`, updates movement in `movePlayer`, and draws the scene in `render`.
3. `windowProc` turns keyboard and mouse events into game actions. `canStand` checks whether the player's circular body can fit at a position; `atFloor` reads the fine collision grid.
4. On a new run, `generateMaze` builds a small polar room graph as a randomized spanning tree. It then rasterizes the curved corridors into a 560 × 560 collision grid. The spanning tree gives every room one route to the exit.
5. Pressing **O** runs `activateRoute`: A* searches the room graph (137 rooms), and the route is drawn through each actual doorway center. This avoids searching or allocating route data for all 313,600 collision cells.
6. `buildMaze` creates walls, the exit frame, and previous-run footprints. `buildFindGeometry` creates only the few batteries and survivor traces that can disappear. Collectibles use a small separate GPU mesh, so finding one never rebuilds the large wall mesh.
7. `render` first draws a depth map for flashlight shadows, then the sky, textured world, small collectible mesh, flashlight, and interface. Static world geometry is uploaded with static GPU storage; only the route and collectible meshes change during play.

## Why the main loops stay quick

| Work | Approximate cost | Why it is bounded |
| --- | --- | --- |
| Build a maze | `O(MapSize² × RingCount)` | The 560 × 560 walkability grid is visited once; ring count is fixed at six. The spanning-tree search itself is `O(V + E)` over 137 rooms. |
| Find a route | `O((V + E) log V)` time and `O(V)` temporary search memory | A* visits the 137-room graph with a priority queue instead of allocating cost and parent arrays for all 313,600 grid cells. |
| Check movement | `O(1)` per collision-grid lookup | Player collision reads a prebuilt grid cell; it does not recalculate the curved maze for each movement step. |
| Collect an item | `O(I)` CPU rebuild for the small item list | A pickup refreshes the separate battery/clue mesh. It does not rebuild the much larger static walls and floor. |
| Draw the HUD | `O(U)` vertices | The UI still draws the visible menu/HUD shapes, but reuses its vector capacity instead of allocating a new buffer on every frame. |

`V` means rooms, `E` means room connections, `I` means visible pickups/clues, and `U` means UI vertices. The map is small and fixed, so complicated caching or parallelism would add maintenance cost without a useful gameplay speed-up.

## Local save files

The game creates these files beside the executable as needed. They are ignored by Git so one player's progress and preferences are not included in a project update.

| Save file | Contents |
| --- | --- |
| `settings.ini` | Display, sound, difficulty, control preferences, and best-time record. |
| `past_runs.csv` | Completed run time, date, route-skill uses, seed, and omen. |
| `last_run_trail.csv` | Up to 2,400 sampled positions from the most recently completed maze. |

## Finding the code

| Function | What it does |
| --- | --- |
| `generateMaze` | Creates the seeded room graph, passages, items, traces, wall arcs, and fine collision grid. |
| `buildMaze` | Makes the mostly static floor, curved walls, fixed rubble, exit geometry, and old footprints. |
| `buildFindGeometry` / `uploadFindGeometry` | Builds and refreshes only the still-visible batteries and survivor traces. |
| `buildEchoGeometry` | Finds nearby walkable floor for previous-run steps; `buildMaze` includes these marks in the static world upload. |
| `activateRoute` | Runs graph-based A* and draws a wall-safe route through doorway centers. |
| `movePlayer` | Reads movement keys, applies collision checks, handles sound/pickups, and detects a win. |
| `collectNearbyFinds` | Grants batteries or clue/note messages when the player reaches an item. |
| `updateFlashlight` / `updateAtmosphere` | Simulates charge and flicker, then updates the moon, clouds, and spatial audio. |
| `loadMap`, `saveSettings`, `loadPastRuns`, `recordPastRun`, `loadPreviousTrail`, `savePreviousTrail` | Load and save the initial settings, run history, best record, and visual trail. |
| `windowProc` | Handles native window events, keyboard controls, and raw mouse look. |
| `initializeRenderer` | Loads assets and creates shaders, textures, buffers, and shadow resources. |
| `render` / `renderUi` | Draw the 3D world and whichever menu or HUD is currently active. |
| `SpatialAudio::fill` | Produces a fixed-size stereo audio block without allocating memory in the audio callback. |

Most code in this first-person prototype remains in one source file so a beginner can follow the complete frame flow without navigating many small modules. The in-code comments explain the intent of the main functions; this table gives a quick map before opening the source.
