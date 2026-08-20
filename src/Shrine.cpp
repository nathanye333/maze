#include "Shrine.h"

bool isNearShrine(GridPosition player, const Shrine& shrine) {
    return manhattan(player, shrine.position) <= 1;
}

Shrine* findNearbyShrine(std::vector<Shrine>& shrines, GridPosition player) {
    for (Shrine& shrine : shrines) {
        if (isNearShrine(player, shrine)) {
            return &shrine;
        }
    }
    return nullptr;
}

const Shrine* findNearbyShrine(const std::vector<Shrine>& shrines, GridPosition player) {
    for (const Shrine& shrine : shrines) {
        if (isNearShrine(player, shrine)) {
            return &shrine;
        }
    }
    return nullptr;
}
