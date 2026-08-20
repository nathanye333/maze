#pragma once

#include "Maze.h"

constexpr int PLAYER_MAX_HEALTH = 100;
constexpr float PLAYER_MOVE_COOLDOWN = 0.12f;

struct Player {
    GridPosition position{};
    int health = PLAYER_MAX_HEALTH;
    float moveCooldown = 0.0f;
};

bool tryMovePlayer(Player& player, const Maze& maze, int dx, int dy);
