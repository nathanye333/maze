#pragma once

#include <optional>
#include <vector>

constexpr int MAZE_WIDTH = 64;
constexpr int MAZE_HEIGHT = 64;

enum class CellType {
    Wall,
    Floor
};

struct GridPosition {
    int x = 0;
    int y = 0;

    bool operator==(const GridPosition& other) const { return x == other.x && y == other.y; }
    bool operator!=(const GridPosition& other) const { return !(*this == other); }
};

int manhattan(GridPosition a, GridPosition b);

class Maze {
public:
    Maze(int width = MAZE_WIDTH, int height = MAZE_HEIGHT);

    int width() const { return width_; }
    int height() const { return height_; }

    bool inBounds(int x, int y) const;
    bool inBounds(GridPosition p) const;
    CellType cellAt(int x, int y) const;
    CellType cellAt(GridPosition p) const;
    void setCell(int x, int y, CellType type);
    bool isWalkable(int x, int y) const;
    bool isWalkable(GridPosition p) const;

    GridPosition start() const { return start_; }
    GridPosition exitPosition() const { return exit_; }
    void setStart(GridPosition p) { start_ = p; }
    void setExit(GridPosition p) { exit_ = p; }

    std::vector<GridPosition> walkableNeighbors(GridPosition p) const;
    int walkableNeighborCount(GridPosition p) const;
    std::vector<GridPosition> floorCells() const;
    std::vector<GridPosition> reachableFloors(GridPosition from) const;

    std::optional<int> bfsDistance(GridPosition from, GridPosition to) const;
    std::optional<GridPosition> nextStepToward(GridPosition from, GridPosition to) const;
    GridPosition farthestFrom(GridPosition from) const;

    bool operator==(const Maze& other) const;

private:
    int index(int x, int y) const { return y * width_ + x; }

    int width_;
    int height_;
    std::vector<CellType> cells_;
    GridPosition start_{};
    GridPosition exit_{};
};
