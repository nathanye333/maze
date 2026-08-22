#pragma once

#include "CombatAnimation.h"
#include "EventSystem.h"
#include "Maze.h"
#include "Weapon.h"
#include "WorldState.h"

bool hasLineOfSight(const Maze& maze, GridPosition from, GridPosition to);

bool applyDamageToEnemy(
    struct Enemy& enemy,
    int damage,
    WorldState& world,
    EventSystem& events,
    int gameTime,
    CombatVisualState& visuals);

bool tryPlayerAttack(WorldState& world, EventSystem& events, int gameTime, CombatVisualState& visuals);

GridPosition computeAimFacing(GridPosition player, float aimWorldX, float aimWorldY, int cellSize);

void updateCombat(WorldState& world, EventSystem& events, int gameTime, float dt, CombatVisualState& visuals);
