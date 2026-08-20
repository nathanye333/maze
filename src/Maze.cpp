#include "Maze.h"

#include <cstdlib>
#include <queue>
#include <vector>

int manhattan(GridPosition a, GridPosition b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

Maze::Maze(int width, int height)
    : width_(width), height_(height), cells_(static_cast<size_t>(width * height), CellType::Wall) {}

bool Maze::inBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < width_ && y < height_;
}

bool Maze::inBounds(GridPosition p) const {
    return inBounds(p.x, p.y);
}

CellType Maze::cellAt(int x, int y) const {
    if (!inBounds(x, y)) {
        return CellType::Wall;
    }
    return cells_[static_cast<size_t>(index(x, y))];
}

CellType Maze::cellAt(GridPosition p) const {
    return cellAt(p.x, p.y);
}

void Maze::setCell(int x, int y, CellType type) {
    if (inBounds(x, y)) {
        cells_[static_cast<size_t>(index(x, y))] = type;
    }
}

bool Maze::isWalkable(int x, int y) const {
    return cellAt(x, y) == CellType::Floor;
}

bool Maze::isWalkable(GridPosition p) const {
    return isWalkable(p.x, p.y);
}

std::vector<GridPosition> Maze::walkableNeighbors(GridPosition p) const {
    static const int kDx[] = {0, 0, -1, 1};
    static const int kDy[] = {-1, 1, 0, 0};
    std::vector<GridPosition> result;
    result.reserve(4);
    for (int i = 0; i < 4; ++i) {
        GridPosition n{p.x + kDx[i], p.y + kDy[i]};
        if (isWalkable(n)) {
            result.push_back(n);
        }
    }
    return result;
}

int Maze::walkableNeighborCount(GridPosition p) const {
    return static_cast<int>(walkableNeighbors(p).size());
}

std::vector<GridPosition> Maze::floorCells() const {
    std::vector<GridPosition> floors;
    floors.reserve(static_cast<size_t>(width_ * height_ / 2));
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (isWalkable(x, y)) {
                floors.push_back(GridPosition{x, y});
            }
        }
    }
    return floors;
}

std::vector<GridPosition> Maze::reachableFloors(GridPosition from) const {
    std::vector<GridPosition> result;
    if (!isWalkable(from)) {
        return result;
    }

    std::vector<char> seen(static_cast<size_t>(width_ * height_), 0);
    std::queue<GridPosition> q;
    q.push(from);
    seen[static_cast<size_t>(index(from.x, from.y))] = 1;

    while (!q.empty()) {
        GridPosition p = q.front();
        q.pop();
        result.push_back(p);
        for (GridPosition n : walkableNeighbors(p)) {
            const size_t i = static_cast<size_t>(index(n.x, n.y));
            if (!seen[i]) {
                seen[i] = 1;
                q.push(n);
            }
        }
    }
    return result;
}

std::optional<int> Maze::bfsDistance(GridPosition from, GridPosition to) const {
    if (!isWalkable(from) || !isWalkable(to)) {
        return std::nullopt;
    }
    if (from == to) {
        return 0;
    }

    std::vector<int> dist(static_cast<size_t>(width_ * height_), -1);
    std::queue<GridPosition> q;
    q.push(from);
    dist[static_cast<size_t>(index(from.x, from.y))] = 0;

    while (!q.empty()) {
        GridPosition p = q.front();
        q.pop();
        const int d = dist[static_cast<size_t>(index(p.x, p.y))];
        for (GridPosition n : walkableNeighbors(p)) {
            const size_t i = static_cast<size_t>(index(n.x, n.y));
            if (dist[i] == -1) {
                dist[i] = d + 1;
                if (n == to) {
                    return dist[i];
                }
                q.push(n);
            }
        }
    }
    return std::nullopt;
}

std::optional<GridPosition> Maze::nextStepToward(GridPosition from, GridPosition to) const {
    if (!isWalkable(from) || !isWalkable(to) || from == to) {
        return std::nullopt;
    }

    std::vector<int> parent(static_cast<size_t>(width_ * height_), -1);
    std::queue<GridPosition> q;
    q.push(from);
    parent[static_cast<size_t>(index(from.x, from.y))] = -2;

    bool found = false;
    while (!q.empty()) {
        GridPosition p = q.front();
        q.pop();
        if (p == to) {
            found = true;
            break;
        }
        for (GridPosition n : walkableNeighbors(p)) {
            const size_t i = static_cast<size_t>(index(n.x, n.y));
            if (parent[i] == -1) {
                parent[i] = index(p.x, p.y);
                q.push(n);
            }
        }
    }

    if (!found) {
        return std::nullopt;
    }

    int current = index(to.x, to.y);
    const int startIndex = index(from.x, from.y);
    int step = current;
    while (parent[static_cast<size_t>(current)] != startIndex && parent[static_cast<size_t>(current)] >= 0) {
        step = parent[static_cast<size_t>(current)];
        current = step;
    }
    if (parent[static_cast<size_t>(current)] == startIndex) {
        step = current;
    }
    return GridPosition{step % width_, step / width_};
}

GridPosition Maze::farthestFrom(GridPosition from) const {
    GridPosition best = from;
    int bestDist = -1;

    if (!isWalkable(from)) {
        for (GridPosition p : floorCells()) {
            from = p;
            break;
        }
        best = from;
    }

    std::vector<int> dist(static_cast<size_t>(width_ * height_), -1);
    std::queue<GridPosition> q;
    q.push(from);
    dist[static_cast<size_t>(index(from.x, from.y))] = 0;

    while (!q.empty()) {
        GridPosition p = q.front();
        q.pop();
        const int d = dist[static_cast<size_t>(index(p.x, p.y))];
        if (d > bestDist) {
            bestDist = d;
            best = p;
        }
        for (GridPosition n : walkableNeighbors(p)) {
            const size_t i = static_cast<size_t>(index(n.x, n.y));
            if (dist[i] == -1) {
                dist[i] = d + 1;
                q.push(n);
            }
        }
    }
    return best;
}

bool Maze::operator==(const Maze& other) const {
    return width_ == other.width_ && height_ == other.height_ && cells_ == other.cells_ &&
           start_ == other.start_ && exit_ == other.exit_;
}
