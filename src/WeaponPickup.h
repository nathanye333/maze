#pragma once

#include "Maze.h"

#include <cstdint>
#include <string>
#include <vector>

struct WeaponPickup {
    GridPosition position{};
    std::string weaponId;
    bool collected = false;
};

class EventSystem;
struct WorldState;

void spawnDefaultWeaponPickups(std::vector<WeaponPickup>& pickups, const Maze& maze, uint32_t seed);
bool tryCollectWeaponPickups(WorldState& world, EventSystem& events, int gameTime);
