# Qwen Maze

A lightweight 2D maze game in C++20. An unseen God observes the player and can regenerate the maze, teleport the player, spawn enemies, or speak. Configure Ollama via a `.env` file (see `.env.example`). Without those settings the game uses a deterministic `MockGod`.

C++ owns the world. The God only returns a structured decision. The game validates that decision, checks power, then mutates state.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release -j
```

Requires C++20, CMake 3.16+, and common Linux raylib dependencies (`libx11-dev`, `libxrandr-dev`, `libxi-dev`, `libxcursor-dev`, `libxinerama-dev`, `libgl1-mesa-dev`). raylib, nlohmann/json, and cpp-httplib are fetched automatically.

## Run

```bash
# Windows (Visual Studio generator)
.\build\Release\qwen-maze.exe

# or Debug
.\build\Debug\qwen-maze.exe
```

### Ollama God

Copy `.env.example` to `.env` (or edit the local `.env`) and set your model:

```env
QWEN_MAZE_GOD=ollama
OLLAMA_MODEL=qwen3.6
OLLAMA_HOST=127.0.0.1:11434
```

Have Ollama running with that model pulled, then run the exe as usual. The game loads `.env` from the working directory (and a few parent folders); builds also copy it next to the executable. Process environment variables still win if already set.

HTTP runs on a worker thread with streaming. While the God thinks, gameplay pauses and the screen flashes in a color based on favor. The wait times out after 60s. Watch the console for `[OllamaGod]` logs (request, HTTP status, raw model JSON, parsed action).

## Tests

```bash
.\build\Debug\qwen-maze-tests.exe
```

## Controls

- **WASD** — move
- **E** — pray at a purple shrine, then type a short message
- **Enter** — send the prayer
- **Esc** — cancel the prayer
- **Space** — kill an adjacent enemy
- **L** — toggle raycast line-of-sight fog
- **F11** — toggle fullscreen (window is also resizable)
- Close the window to quit

## World

Mazes are built from randomly placed rooms connected by Prim MST hallways (seeded, always solvable). With LOS on, only tiles reached by wall-blocked Bresenham rays within Manhattan distance 15 are drawn.

## Colors

- Walls: dark gray
- Player: blue
- Enemies: red
- Exit: green
- Shrines: purple

## God

Besides shrine prayers, the God is called on a random interval (20–45 seconds). Actions cost power (message 0, spawn 1, teleport 2, regenerate 4). Power starts at 10 and regenerates 1 every 30 seconds, capped at 10. Favor starts at 0, ranges from -50 to 50, and can shift when you pray or when the God replies.

Invalid requests are rejected. The God cannot place the player in a wall, spawn on the player, exceed 10 enemies, or spend more power than it has.
