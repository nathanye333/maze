#include "WeaponPickup.h"

#include "EventSystem.h"
#include "Weapon.h"
#include "WorldState.h"

#include <algorithm>
#include <random>
#include <sstream>

void spawnDefaultWeaponPickups(std::vector<WeaponPickup>& pickups, const Maze& maze, uint32_t seed) {
    pickups.clear();
    std::mt19937 rng(seed ^ 0xC0FFEE42u);
    auto spots = maze.reachableFloors(maze.start());
    spots.erase(
        std::remove_if(
            spots.begin(),
            spots.end(),
            [&](GridPosition p) { return p == maze.start() || p == maze.exitPosition(); }),
        spots.end());
    std::shuffle(spots.begin(), spots.end(), rng);

    size_t index = 0;
    if (index < spots.size()) {
        pickups.push_back(WeaponPickup{spots[index++], "sword", false});
    }
    if (index < spots.size()) {
        pickups.push_back(WeaponPickup{spots[index++], "gun", false});
    }
}

bool tryCollectWeaponPickups(WorldState& world, EventSystem& events, int gameTime) {
    bool collectedAny = false;
    for (WeaponPickup& pickup : world.weaponPickups) {
        if (pickup.collected || pickup.position != world.player.position) {
            continue;
        }
        if (!addWeaponToInventory(world.inventory, pickup.weaponId)) {
            continue;
        }
        pickup.collected = true;
        collectedAny = true;
        const WeaponDef& weapon = weaponDefById(pickup.weaponId);
        std::ostringstream desc;
        desc << "Player picked up " << weapon.name << " at (" << pickup.position.x << ", "
             << pickup.position.y << ").";
        events.record(EventType::WeaponPickedUp, gameTime, desc.str());
    }
    if (collectedAny) {
        syncRecentEvents(world, events);
    }
    return collectedAny;
}
