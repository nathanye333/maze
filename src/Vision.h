#pragma once

#include "Maze.h"

#include <vector>

constexpr int VISIBILITY_RADIUS = 15;

// Bresenham-style raycast fog of war. Walls block further sight along a ray.
std::vector<char> computeVisibility(const Maze& maze, GridPosition origin, int radius = VISIBILITY_RADIUS);

bool isVisible(const std::vector<char>& visibility, const Maze& maze, int x, int y);
bool isVisible(const std::vector<char>& visibility, const Maze& maze, GridPosition p);
