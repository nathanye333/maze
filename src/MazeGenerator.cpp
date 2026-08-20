#include "MazeGenerator.h"

#include <algorithm>
#include <array>
#include <random>
#include <stack>

namespace {

struct Dir {
    int dx;
    int dy;
};

Maze carveMaze(int width, int height, uint32_t seed) {
    Maze maze(width, height);
    if (width < 3 || height < 3) {
        return maze;
    }

    std::mt19937 rng(seed);
    constexpr Dir kDirs[] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    const int startX = 1;
    const int startY = 1;
    maze.setCell(startX, startY, CellType::Floor);

    std::stack<GridPosition> stack;
    stack.push(GridPosition{startX, startY});

    while (!stack.empty()) {
        GridPosition current = stack.top();
        std::array<int, 4> order = {0, 1, 2, 3};
        std::shuffle(order.begin(), order.end(), rng);

        bool carved = false;
        for (int i : order) {
            const int nx = current.x + kDirs[i].dx * 2;
            const int ny = current.y + kDirs[i].dy * 2;
            if (maze.inBounds(nx, ny) && maze.cellAt(nx, ny) == CellType::Wall && nx > 0 && ny > 0 &&
                nx < width - 1 && ny < height - 1) {
                maze.setCell(current.x + kDirs[i].dx, current.y + kDirs[i].dy, CellType::Floor);
                maze.setCell(nx, ny, CellType::Floor);
                stack.push(GridPosition{nx, ny});
                carved = true;
                break;
            }
        }
        if (!carved) {
            stack.pop();
        }
    }

    GridPosition start{startX, startY};
    if (!maze.isWalkable(start)) {
        for (GridPosition p : maze.floorCells()) {
            start = p;
            break;
        }
    }
    maze.setStart(start);
    maze.setExit(maze.farthestFrom(start));
    return maze;
}

}  // namespace

Maze MazeGenerator::generate(int width, int height, uint32_t seed) {
    for (int attempt = 0; attempt < 32; ++attempt) {
        const uint32_t trySeed = seed + static_cast<uint32_t>(attempt);
        Maze maze = carveMaze(width, height, trySeed);
        if (isSolvable(maze, maze.start(), maze.exitPosition())) {
            return maze;
        }
    }
    return carveMaze(width, height, seed);
}

bool MazeGenerator::isSolvable(const Maze& maze, GridPosition start, GridPosition end) {
    return maze.bfsDistance(start, end).has_value();
}
