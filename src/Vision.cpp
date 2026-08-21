#include "Vision.h"

#include <algorithm>
#include <cstdlib>

namespace {

void markVisible(std::vector<char>& visible, const Maze& maze, int x, int y) {
    if (maze.inBounds(x, y)) {
        visible[static_cast<size_t>(y * maze.width() + x)] = 1;
    }
}

void castRay(std::vector<char>& visible, const Maze& maze, GridPosition origin, int targetX, int targetY, int radius) {
    const int dx = std::abs(targetX - origin.x);
    const int dy = std::abs(targetY - origin.y);
    int error = dx - dy;
    const int xInc = targetX < origin.x ? -1 : 1;
    const int yInc = targetY < origin.y ? -1 : 1;

    int currX = origin.x;
    int currY = origin.y;

    while (true) {
        if (!maze.inBounds(currX, currY)) {
            break;
        }
        if (std::abs(currX - origin.x) + std::abs(currY - origin.y) > radius) {
            break;
        }

        markVisible(visible, maze, currX, currY);
        if (maze.cellAt(currX, currY) == CellType::Wall) {
            break;
        }
        if (currX == targetX && currY == targetY) {
            break;
        }

        const int error2 = 2 * error;
        // Widen diagonal steps slightly so room corners don't leave odd gaps.
        if (error2 > -dy && error2 < dx) {
            markVisible(visible, maze, currX + xInc, currY);
            markVisible(visible, maze, currX, currY + yInc);
        }

        bool stepped = false;
        if (error2 > -dy) {
            error -= dy;
            currX += xInc;
            stepped = true;
        }
        if (error2 < dx) {
            error += dx;
            currY += yInc;
            stepped = true;
        }
        if (!stepped) {
            break;
        }
    }
}

}  // namespace

std::vector<char> computeVisibility(const Maze& maze, GridPosition origin, int radius) {
    std::vector<char> visible(static_cast<size_t>(maze.width() * maze.height()), 0);
    if (!maze.inBounds(origin)) {
        return visible;
    }

    markVisible(visible, maze, origin.x, origin.y);

    const int minX = std::max(0, origin.x - radius);
    const int maxX = std::min(maze.width() - 1, origin.x + radius);
    const int minY = std::max(0, origin.y - radius);
    const int maxY = std::min(maze.height() - 1, origin.y + radius);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (std::abs(x - origin.x) + std::abs(y - origin.y) > radius) {
                continue;
            }
            castRay(visible, maze, origin, x, y, radius);
        }
    }

    return visible;
}

bool isVisible(const std::vector<char>& visibility, const Maze& maze, int x, int y) {
    if (!maze.inBounds(x, y)) {
        return false;
    }
    return visibility[static_cast<size_t>(y * maze.width() + x)] != 0;
}

bool isVisible(const std::vector<char>& visibility, const Maze& maze, GridPosition p) {
    return isVisible(visibility, maze, p.x, p.y);
}
