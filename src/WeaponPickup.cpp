#include "WeaponPickup.h"

#include "EventSystem.h"
#include "Shrine.h"
#include "Weapon.h"
#include "WorldState.h"

#include <algorithm>
#include <random>
#include <sstream>

namespace {

bool isExcludedPosition(GridPosition position, const std::vector<GridPosition>& excluded) {
    return std::find(excluded.begin(), excluded.end(), position) != excluded.end();
}

bool isShrineAt(const std::vector<Shrine>& shrines, GridPosition position) {
    for (const Shrine& shrine : shrines) {
        if (shrine.position == position) {
            return true;
        }
    }
    return false;
}

WeaponPickup* findMutableWeaponPickup(std::vector<WeaponPickup>& pickups, GridPosition position) {
    for (WeaponPickup& pickup : pickups) {
        if (!pickup.collected && pickup.position == position) {
            return &pickup;
        }
    }
    return nullptr;
}

}  // namespace

void spawnDefaultWeaponPickups(
    std::vector<WeaponPickup>& pickups,
    const Maze& maze,
    uint32_t seed,
    const std::vector<GridPosition>& excluded) {
    pickups.clear();
    std::mt19937 rng(seed ^ 0xC0FFEE42u);
    auto spots = maze.reachableFloors(maze.start());
    spots.erase(
        std::remove_if(
            spots.begin(),
            spots.end(),
            [&](GridPosition p) {
                return p == maze.start() || p == maze.exitPosition() || isExcludedPosition(p, excluded);
            }),
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

const WeaponPickup* findWeaponPickupAt(const std::vector<WeaponPickup>& pickups, GridPosition position) {
    for (const WeaponPickup& pickup : pickups) {
        if (!pickup.collected && pickup.position == position) {
            return &pickup;
        }
    }
    return nullptr;
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

bool tryDropEquippedWeapon(WorldState& world, EventSystem& events, int gameTime) {
    if (findWeaponPickupAt(world.weaponPickups, world.player.position) != nullptr) {
        return false;
    }

    std::string weaponId;
    if (!dropEquippedWeapon(world.inventory, weaponId)) {
        return false;
    }

    for (WeaponPickup& pickup : world.weaponPickups) {
        if (pickup.weaponId == weaponId && pickup.collected) {
            pickup.collected = false;
            pickup.position = world.player.position;
            const WeaponDef& weapon = weaponDefById(weaponId);
            std::ostringstream desc;
            desc << "Player dropped " << weapon.name << " at (" << world.player.position.x << ", "
                 << world.player.position.y << ").";
            events.record(EventType::WeaponDropped, gameTime, desc.str());
            syncRecentEvents(world, events);
            return true;
        }
    }

    world.weaponPickups.push_back(WeaponPickup{world.player.position, weaponId, false});
    const WeaponDef& weapon = weaponDefById(weaponId);
    std::ostringstream desc;
    desc << "Player dropped " << weapon.name << " at (" << world.player.position.x << ", "
         << world.player.position.y << ").";
    events.record(EventType::WeaponDropped, gameTime, desc.str());
    syncRecentEvents(world, events);
    return true;
}

bool isWeaponPickupOnShrine(const std::vector<WeaponPickup>& pickups, const std::vector<Shrine>& shrines) {
    for (const WeaponPickup& pickup : pickups) {
        if (!pickup.collected && isShrineAt(shrines, pickup.position)) {
            return true;
        }
    }
    return false;
}
