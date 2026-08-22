#pragma once

#include "Maze.h"

#include <vector>

enum class CombatAnimKind {
    SwordSwing,
    GunMuzzleFlash,
    HitFlash
};

struct CombatAnimation {
    CombatAnimKind kind = CombatAnimKind::SwordSwing;
    float elapsed = 0.0f;
    float duration = 0.0f;
    GridPosition origin{};
    GridPosition facing{};
};

struct CombatVisualState {
    std::vector<CombatAnimation> active;
};

void spawnCombatAnimation(
    CombatVisualState& visuals,
    CombatAnimKind kind,
    GridPosition origin,
    GridPosition facing = {});

void updateCombatAnimations(CombatVisualState& visuals, float dt);
