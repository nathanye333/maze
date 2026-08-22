#pragma once

#include "Maze.h"

#include <cstdint>
#include <string>
#include <vector>

struct Shrine;

struct WeaponPickup {
    GridPosition position{};
    std::string weaponId;
    bool collected = false;
};

class EventSystem;
struct WorldState;

void spawnDefaultWeaponPickups(
    std::vector<WeaponPickup>& pickups,
    const Maze& maze,
    uint32_t seed,
    const std::vector<GridPosition>& excluded);
const WeaponPickup* findWeaponPickupAt(const std::vector<WeaponPickup>& pickups, GridPosition position);
bool tryCollectWeaponPickups(WorldState& world, EventSystem& events, int gameTime);
bool tryDropEquippedWeapon(WorldState& world, EventSystem& events, int gameTime);
bool isWeaponPickupOnShrine(const std::vector<WeaponPickup>& pickups, const std::vector<Shrine>& shrines);
