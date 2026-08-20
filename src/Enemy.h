#pragma once

#include "Maze.h"

#include <random>
#include <vector>

constexpr int MAX_ENEMIES = 10;
constexpr int INITIAL_ENEMY_COUNT = 5;
constexpr int ENEMY_DETECTION_RANGE = 8;
constexpr int ENEMY_DAMAGE = 10;
constexpr float ENEMY_MOVE_COOLDOWN = 0.28f;
constexpr float ENEMY_ATTACK_COOLDOWN = 0.6f;

struct Enemy {
    int id = 0;
    GridPosition position{};
    bool active = true;
    float moveCooldown = 0.0f;
    float attackCooldown = 0.0f;
};

int activeEnemyCount(const std::vector<Enemy>& enemies);
GridPosition chooseEnemyMove(const Enemy& enemy, const Maze& maze, GridPosition playerPos, std::mt19937& rng);
