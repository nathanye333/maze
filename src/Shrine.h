#pragma once

#include "Maze.h"

#include <vector>

constexpr int SHRINE_COUNT = 3;

struct Shrine {
    GridPosition position{};
    bool visited = false;
    bool reachedAnnounced = false;
};

bool isNearShrine(GridPosition player, const Shrine& shrine);
Shrine* findNearbyShrine(std::vector<Shrine>& shrines, GridPosition player);
const Shrine* findNearbyShrine(const std::vector<Shrine>& shrines, GridPosition player);
