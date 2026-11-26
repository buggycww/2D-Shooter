# 2D-Shooter

A simple 2D shooter build using C++ and SDL3. This is a learning project I made by following YouTube tutorials to develop a game without the help of a game engine, mainly to understand how a game actually handles rendering, assets, collision, update loops, cameras, maps, and parsing at a lower level.

The collision system is adapted from my existing [quadtree-visualiser](https://github.com/buggycww/quadtree-visualiser), with slight modifications to accommodate this project's needs. Entries store a full AABB instead of only a point, and Point is repurposed as an identity handle, so a query result can be mapped back to the game object that owns it.

## Project Overview

- **Rendering** - Setting up a window, drawing sprites, and managing the render loop with SDL3.
- **Assets** - Loading textures, spritesheets, and audio from the `data/` directory.
- **Collision** - AABB collision with separate-axis handling, plus a custom quadtree for broad-phase collision detection, core logic imported from [quadtree-visualiser](https://github.com/buggycww/quadtree-visualiser).
- **Update cycle** - A game loop that processes input, updates entities, and renders each frame.
- **Camera** - Following the player and controlling what portion of the map is visible.
- **Maps** - Parsed tilemaps loaded from data files (upgraded from initial hard-coded layouts).
- **Parsing** - Reading XML map and configuration data using tinyxml2.
- **Animation** - Sprite animation state switching and frame management.
- **Enemies and audio** - Basic enemy behavior and sound playback.

## Language & Libraries

- **C++**
- **SDL3** - Windowing, rendering, input, and audio
- **tinyxml2** - XML parsing for tilemaps and data files
- **CMake** - Build configuration

## Controls

Input is handled through SDL3, with bindings configured manually in the input code:

| Action | Input |
| --- | --- |
| Move left | A |
| Move right | D |
| Jump | Space |
| Shoot | Left mouse button |

## Build

1. Clone the repository:

   ```bash
   git clone https://github.com/buggycww/2D-Shooter.git
   ```

2. Install the required dependencies: SDL3 and CMake. On Windows, the project can be configured with the included CMake presets.

3. Configure and build with CMake:

   ```bash
   cmake -B build
   cmake --build build
   ```

   Or use the CMake presets if your environment supports them.

4. Run the produced executable. The game expects the `data/` directory to be available relative to the working directory.

## Notes

- This is a learning project, not a production game. The code favors clarity and experimentation, so it's not really polished.
- The quadtree broad-phase collision system imported from [quadtree-visualiser](https://github.com/buggycww/quadtree-visualiser), and replaced the earlier naive collision checks (checking each object against every other object).
- Tilemaps are parsed from data files rather than embedded in code.
