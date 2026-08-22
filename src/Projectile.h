#pragma once

#include "Maze.h"

#include <vector>

constexpr int MAX_PROJECTILES = 16;

struct Projectile {
    int id = 0;
    GridPosition cell{};
    GridPosition direction{};
    int damage = 0;
    bool active = false;
    float moveTimer = 0.0f;
    float speed = 12.0f;
    int distanceTravelled = 0;
    int maxRange = 0;
};

class EventSystem;
struct Enemy;
struct WorldState;
struct CombatVisualState;

void spawnProjectile(
    std::vector<Projectile>& projectiles,
    int& nextProjectileId,
    GridPosition origin,
    GridPosition direction,
    int damage,
    float speed,
    int maxRange);

void updateProjectiles(
    std::vector<Projectile>& projectiles,
    const Maze& maze,
    std::vector<Enemy>& enemies,
    int gameTime,
    float dt,
    EventSystem& events,
    WorldState& world,
    CombatVisualState& visuals);
