#pragma once

#include "Maze.h"

#include <cstdint>

class MazeGenerator {
public:
    Maze generate(int width, int height, uint32_t seed);
    bool isSolvable(const Maze& maze, GridPosition start, GridPosition end);
};
