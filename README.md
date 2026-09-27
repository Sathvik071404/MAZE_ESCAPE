# Maze Escape

A first-person OpenGL maze game built directly on Win32/WGL. `assets/maze.png` is rasterized into the walkable layout in `assets/maze.map`; the game generates its 3D wall mesh from that layout at startup.

## Build and run

Use a Windows C++ compiler and CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run `build\Release\MazeEscape.exe` with a Visual Studio generator, or `build\MazeEscape.exe` with a single-configuration generator. CMake copies maze assets next to the executable.

## Play from the GitHub ZIP

Download and extract the repository ZIP, then double-click `MazeEscape.exe` in the extracted folder. Keep the `assets` folder beside the executable; it contains the maze and textures. No build tools are needed.

## Controls

- **Enter / Space / Click**: start
- **W / S**: move forward/back
- **A / D**: strafe
- **Shift**: sprint
- **Mouse**: look around
- **O**: reveal the route for one second (15-second cooldown)
- **Esc**: pause; Esc again returns to the title screen
- **Click**: start or resume
- **R**: restart after escaping

Find the green exit arch. Walls block the view; the flashlight is a short spotlight, and the timer stops when you escape. The route skill appears at the lower right while playing.

## Textures

The bundled leafy grass, weathered gray block wall, and moonlit sky are converted to PPM for the built-in OpenGL loader. Source assets are CC0 from [Poly Haven Leafy Grass](https://polyhaven.com/a/leafy_grass), [Old Stone Wall](https://polyhaven.com/a/old_stone_wall), and [Qwantani Moon Noon (Pure Sky)](https://polyhaven.com/a/qwantani_moon_noon_puresky). The wall shader lightens the stone and adds scattered moss patches; the source texture includes worn joints and small cavities.
