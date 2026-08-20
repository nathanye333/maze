# Qwen Maze

A lightweight 2D maze game in C++20. An unseen God observes the player and can regenerate the maze, teleport the player, spawn enemies, or speak. This MVP uses a deterministic `MockGod`. A future local Qwen client can replace it without changing the simulation.

C++ owns the world. The God only returns a structured decision. The game validates that decision, checks power, then mutates state.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

Requires C++20, CMake 3.16+, and common Linux raylib dependencies (`libx11-dev`, `libxrandr-dev`, `libxi-dev`, `libxcursor-dev`, `libxinerama-dev`, `libgl1-mesa-dev`). raylib and nlohmann/json are fetched automatically.

## Run

```bash
./build/qwen-maze
```

## Tests

```bash
./build/qwen-maze-tests
```

## Controls

- **WASD** — move
- **E** — speak to the God when next to a purple shrine
- **Space** — kill an adjacent enemy
- Close the window to quit

## Colors

- Walls: dark gray
- Player: blue
- Enemies: red
- Exit: green
- Shrines: purple

## God

The God is evaluated every 15 seconds and after important events (shrine, near exit, death, several kills). Actions cost power (message 0, spawn 1, teleport 2, regenerate 4). Power starts at 10 and regenerates 1 every 30 seconds, capped at 10.

Invalid requests are rejected. The God cannot place the player in a wall, spawn on the player, exceed 10 enemies, or spend more power than it has.

To swap in Qwen later, implement `God` (same `evaluate(const WorldState&)`) and construct it in `main` instead of `MockGod`. `toJson(world)` already matches the planned request payload.
