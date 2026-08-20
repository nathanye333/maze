# Build: Qwen Maze — C++ MVP

Build a lightweight 2D maze game in C++ where an on-device Qwen model will eventually act as an unseen "God" that observes the player's behavior and can manipulate the maze.

## Core Goal

Create a playable MVP with:

* A large 2D procedurally generated maze
* A player controlled with WASD
* Several simple enemies that chase the player
* An exit somewhere in the maze
* A few shrine locations
* A basic event system that records meaningful player actions
* A God interface with four possible actions:

  1. regenerate the maze
  2. teleport the player
  3. spawn an enemy
  4. send the player a message
* For now, DO NOT integrate Qwen yet. Create a mock God implementation with the exact same interface that can later be replaced with a local Qwen HTTP client.

The game should prioritize simplicity and performance over graphics.

---

# Technology

Use:

* C++20
* CMake
* raylib for rendering/input
* nlohmann/json for serialization
* Standard C++ library wherever possible

Do NOT use:

* Unreal Engine
* Unity
* OpenGL directly
* complex game engines
* networking libraries unless required for the future Qwen interface

The game should run comfortably on a normal laptop and use negligible GPU resources compared with an LLM.

---

# Project Structure

Create a clean project structure:

```text
qwen-maze/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── Game.h
│   ├── Game.cpp
│   ├── Maze.h
│   ├── Maze.cpp
│   ├── Player.h
│   ├── Player.cpp
│   ├── Enemy.h
│   ├── Enemy.cpp
│   ├── Shrine.h
│   ├── Shrine.cpp
│   ├── Event.h
│   ├── Event.cpp
│   ├── EventSystem.h
│   ├── EventSystem.cpp
│   ├── God.h
│   ├── God.cpp
│   ├── MockGod.h
│   ├── MockGod.cpp
│   ├── WorldState.h
│   ├── WorldState.cpp
│   ├── MazeGenerator.h
│   └── MazeGenerator.cpp
├── assets/
│   └── README.md
└── tests/
    ├── MazeTests.cpp
    └── EventTests.cpp
```

Keep responsibilities separated.

---

# 1. Maze

Use a grid-based maze.

Default dimensions:

```cpp
constexpr int MAZE_WIDTH = 64;
constexpr int MAZE_HEIGHT = 64;
```

Represent each cell as either:

```cpp
enum class CellType {
    Wall,
    Floor
};
```

Generate the maze procedurally using recursive backtracking or another simple reliable algorithm.

Requirements:

* Maze must always be solvable.
* There must always be a valid path from player spawn to exit.
* Player spawn and exit must be placed on floor cells.
* Maze generation should be deterministic when given a seed.
* Support regenerating the maze with a new seed.

After generating a maze, validate it with BFS:

```text
player spawn -> exit
```

If no path exists, regenerate.

Expose:

```cpp
class MazeGenerator {
public:
    Maze generate(int width, int height, uint32_t seed);
    bool isSolvable(const Maze& maze, GridPosition start, GridPosition end);
};
```

---

# 2. Rendering

Keep graphics extremely simple.

Do NOT create detailed art.

Render:

* Walls = dark gray rectangles
* Floor = black/dark background
* Player = blue square
* Enemies = red squares
* Exit = green square
* Shrines = purple square

Use a camera so the 64x64 maze can be larger than the screen.

The player should see only a reasonable portion of the maze around them.

Target:

```text
60 FPS
```

---

# 3. Player

Create:

```cpp
struct Player {
    GridPosition position;
    int health;
};
```

Start with:

```cpp
health = 100;
```

Movement:

```text
W = up
A = left
S = down
D = right
```

Movement should be grid based for simplicity.

Do not allow movement into walls.

Add a small movement cooldown or interpolation if needed so movement feels responsive but remains simple.

---

# 4. Enemies

Create simple enemies.

Each enemy has:

```cpp
struct Enemy {
    int id;
    GridPosition position;
    bool active;
};
```

Start with 5 enemies.

Enemy behavior:

### Patrol

Move randomly through nearby valid floor cells.

### Chase

If the enemy can detect the player, move toward the player using BFS or simple pathfinding.

Detection can initially be simple:

```text
Manhattan distance <= 8
```

No sophisticated AI is necessary.

If an enemy reaches the player:

```text
player.health -= damage
```

Use a simple damage value such as 10.

If health reaches 0:

```text
player dies
```

For now, restart the current maze rather than implementing a complex death system.

---

# 5. Exit

Place one exit in the maze.

When the player reaches the exit:

Display:

```text
YOU ESCAPED
```

Pause the game.

Do not implement complicated endings yet.

---

# 6. Shrines

Place 3 shrines randomly in reachable locations.

When the player is adjacent to a shrine, display:

```text
Press E to speak to the God
```

Pressing E should generate:

```cpp
EventType::PlayerCommunicatedWithGod
```

For the MVP, the MockGod can respond with a random/simple message.

Example:

```text
"You are being watched."
"I wonder why you continue."
"Keep going."
"Do you really believe the exit is real?"
```

---

# 7. Event System

Create an event system.

Events should represent meaningful actions rather than every frame.

Example:

```cpp
enum class EventType {
    MazeStarted,
    PlayerMoved,
    EnemyKilled,
    PlayerDamaged,
    PlayerDied,
    PlayerReachedShrine,
    PlayerCommunicatedWithGod,
    PlayerReachedExit,
    MazeRegenerated,
    PlayerTeleported,
    EnemySpawned,
    GodMessageSent
};
```

Do NOT log every frame.

Record only meaningful events.

Each event should contain enough information for a future LLM to understand it.

For example:

```cpp
struct GameEvent {
    EventType type;
    int gameTime;
    std::string description;
};
```

Example:

```text
PLAYER_REACHED_SHRINE:
"Player reached shrine at (31, 42)."
```

Maintain a bounded recent-event history.

Maximum:

```cpp
constexpr int MAX_RECENT_EVENTS = 30;
```

---

# 8. World State

Create a central WorldState object.

It should contain:

```cpp
struct WorldState {
    Maze maze;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Shrine> shrines;

    int currentLevel;
    uint32_t mazeSeed;

    std::vector<GameEvent> recentEvents;

    int godFavor;
    int godPower;
};
```

Start with:

```text
godFavor = 0
godPower = 10
```

These values should not initially be displayed to the player.

---

# 9. God Interface

This is extremely important.

Create an abstract interface:

```cpp
class God {
public:
    virtual ~God() = default;

    virtual GodDecision evaluate(
        const WorldState& world
    ) = 0;
};
```

Define:

```cpp
enum class GodActionType {
    None,
    RegenerateMaze,
    TeleportPlayer,
    SpawnEnemy,
    SendMessage
};
```

And:

```cpp
struct GodDecision {
    GodActionType action;

    std::string parameter;
    std::string message;

    int powerCost;
};
```

The game should not care whether the decision came from a mock AI or Qwen.

This abstraction is critical because the real implementation will eventually be:

```text
QwenGod
```

instead of:

```text
MockGod
```

---

# 10. God Powers

Implement exactly four powers.

## Regenerate maze

```cpp
regenerate_maze()
```

Generate a completely new valid maze.

Requirements:

* Player gets placed in a valid starting location.
* Exit gets placed somewhere reachable.
* Enemies get redistributed.
* Shrines get redistributed.
* New seed.
* Record a MazeRegenerated event.

The player should NOT be killed when this happens.

---

## Teleport player

Support these parameters:

```text
random_safe
dead_end
far_from_exit
near_enemy
```

Do NOT allow arbitrary coordinates from the God.

C++ should choose the actual valid position.

Example:

```cpp
teleportPlayer("dead_end");
```

Find a valid floor cell that is a dead end and place the player there.

Record:

```text
PLAYER_TELEPORTED
```

---

## Spawn enemy

Support:

```text
random
near_player
far_from_player
```

Place an enemy only on a valid floor cell.

Never spawn an enemy directly on the player.

Enforce:

```cpp
constexpr int MAX_ENEMIES = 10;
```

---

## Send message

Display a God message on screen.

Example:

```text
THE GOD:
"You are getting close."
```

The message should remain visible for several seconds.

---

# 11. MockGod

Implement a deterministic MockGod so the game can be tested without an LLM.

For example:

```text
Every 20 seconds:
    inspect recent events

If player is very close to exit:
    regenerate maze

If player has taken lots of damage:
    sometimes send message

If player has killed many enemies:
    sometimes spawn enemy

Otherwise:
    do nothing
```

Do not make it completely random.

It should demonstrate that the God can observe the world and intervene.

---

# 12. God Evaluation Timing

DO NOT call the God every frame.

Instead, evaluate periodically.

For the MVP:

```cpp
constexpr float GOD_EVALUATION_INTERVAL = 15.0f;
```

Every 15 seconds:

```text
WorldState
    ↓
God.evaluate()
    ↓
GodDecision
    ↓
Game validates decision
    ↓
Game executes action
```

Also trigger an evaluation when important events happen:

* Player reaches shrine
* Player gets close to exit
* Player dies
* Player kills several enemies

But avoid repeatedly calling the God for the same event.

---

# 13. God Power

Give the God a limited intervention budget.

Start with:

```text
God Power = 10
```

Costs:

```text
Send message       0
Spawn enemy        1
Teleport           2
Regenerate maze    4
```

Before executing an action:

```cpp
if (godPower >= decision.powerCost)
```

Otherwise reject the action.

Subtract the cost when successfully executed.

Regenerate 1 power every 30 seconds, up to a maximum of 10.

This ensures the God cannot constantly interfere with the player.

---

# 14. Player behavior statistics

Add a small behavior profile.

```cpp
struct PlayerProfile {
    int enemiesKilled;
    int damageTaken;
    int deaths;
    int shrinesVisited;
    int godInteractions;
    int mazeRegenerationsExperienced;
    int distanceTravelled;
};
```

Don't use an LLM to calculate these.

C++ tracks them deterministically.

This will eventually be sent to Qwen.

---

# 15. Future Qwen integration

DO NOT implement this yet, but design the code so that replacing MockGod is easy.

Eventually:

```text
C++ Game
   |
   | JSON over localhost
   v
Python Qwen server
   |
   v
Qwen
```

The JSON request should eventually contain:

```json
{
  "player": {
    "health": 70,
    "position": [31, 42]
  },
  "profile": {
    "enemies_killed": 3,
    "damage_taken": 20,
    "shrines_visited": 1,
    "god_interactions": 2
  },
  "maze": {
    "width": 64,
    "height": 64,
    "distance_to_exit": 47
  },
  "god": {
    "favor": 25,
    "power": 7
  },
  "recent_events": [
    "Player ignored a shrine.",
    "Player killed an enemy.",
    "Player discovered a dead end."
  ]
}
```

Qwen will eventually return something equivalent to:

```json
{
  "action": "teleport_player",
  "parameter": "dead_end",
  "message": "",
  "power_cost": 2,
  "reason": "The player is becoming too comfortable."
}
```

The C++ game validates everything before executing it.

---

# 16. Critical security/design rule

Treat Qwen as an untrusted decision-maker.

Never allow the model to directly mutate C++ state.

The flow must ALWAYS be:

```text
Qwen
 ↓
structured decision
 ↓
JSON/schema validation
 ↓
C++ validation
 ↓
power check
 ↓
execute
```

For example, if Qwen eventually returns:

```text
teleport_player("outside_map")
```

C++ rejects it.

If it requests:

```text
spawn_enemy()
```

when 10 enemies already exist, reject it.

The model can **request** actions. The game decides whether they're legal.

---

# 17. UI

Keep the UI minimal.

Top-left:

```text
HP: 80
```

Bottom-center when relevant:

```text
[WASD] Move    [E] Interact
```

When God speaks:

```text
┌─────────────────────────────────────┐
│ THE GOD                              │
│                                     │
│ "You are getting closer."           │
└─────────────────────────────────────┘
```

Do not add menus, inventory screens, quest logs, etc.

---

# 18. Testing

Add tests for the important non-rendering systems.

At minimum:

### Maze tests

* Generated maze is solvable.
* Start is floor.
* Exit is floor.
* Regenerated maze is solvable.
* Different seeds produce different mazes.

### God tests

* God cannot exceed max power.
* Actions cost the correct amount.
* Teleport never puts player inside a wall.
* Spawn never puts enemy inside a wall.
* Spawn never puts enemy directly on player.
* Regenerate always creates a solvable maze.
* Invalid God actions are rejected.

---

# 19. Build order

Do NOT attempt everything simultaneously.

Implement in this order:

### Step 1

CMake + raylib project that opens a window.

### Step 2

Maze generation + rendering.

### Step 3

Player movement + collision.

### Step 4

Exit + win condition.

### Step 5

Enemies + basic pathfinding.

### Step 6

Shrines.

### Step 7

Event system.

### Step 8

WorldState.

### Step 9

God interface.

### Step 10

MockGod.

### Step 11

God powers.

### Step 12

God messages + basic UI.

### Step 13

Tests.

Only after all of this works should we implement the actual Qwen server.

---

# Definition of Done

The MVP is complete when I can launch the game and:

1. Enter a large procedurally generated maze.
2. Move around with WASD.
3. See enemies.
4. Enemies can chase me.
5. Find shrines.
6. Reach the exit.
7. Experience the MockGod observing my behavior.
8. The God can:

   * regenerate the maze,
   * teleport me,
   * spawn an enemy,
   * send a message.
9. The God has limited power.
10. Every intervention is recorded as an event.
11. The game never generates an unsolvable maze.
12. The entire game runs locally without requiring an internet connection or an LLM.

## Important implementation philosophy

Keep the code simple.

Do not over-engineer the ECS, rendering system, AI system, or networking.

The important architectural boundary is:

```text
        C++ owns reality

             ↓

     WorldState / Events

             ↓

      God makes decisions

             ↓

      C++ validates them

             ↓

       C++ changes reality
```

The eventual Qwen model should be replaceable without changing the game simulation.

Start implementing now. After each major phase, compile and run the project before moving to the next phase. Do not generate placeholder systems for future features unless they are needed for the current phase.
