#include "MazeGenerator.h"

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

namespace {

struct Room {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    int centerX() const { return x + w / 2; }
    int centerY() const { return y + h / 2; }
};

constexpr int kMinRoomWidth = 4;
constexpr int kRoomWidthVariability = 8;
constexpr int kMinRoomHeight = 4;
constexpr int kRoomHeightVariability = 8;
constexpr int kMinSpacePercent = 30;
constexpr int kSpacePercentVariability = 30;
constexpr int kMaxPlacementAttempts = 1000;

bool roomFits(const Maze& maze, const std::vector<char>& claimed, const Room& room) {
    if (room.x < 1 || room.y < 1) {
        return false;
    }
    if (room.x + room.w >= maze.width() - 1 || room.y + room.h >= maze.height() - 1) {
        return false;
    }
    for (int y = room.y; y <= room.y + room.h; ++y) {
        for (int x = room.x; x <= room.x + room.w; ++x) {
            if (claimed[static_cast<size_t>(y * maze.width() + x)]) {
                return false;
            }
        }
    }
    return true;
}

void claimRoom(std::vector<char>& claimed, int width, const Room& room) {
    for (int y = room.y; y <= room.y + room.h; ++y) {
        for (int x = room.x; x <= room.x + room.w; ++x) {
            claimed[static_cast<size_t>(y * width + x)] = 1;
        }
    }
}

void fillRoom(Maze& maze, const Room& room) {
    for (int y = room.y; y <= room.y + room.h; ++y) {
        for (int x = room.x; x <= room.x + room.w; ++x) {
            const bool border = x == room.x || y == room.y || x == room.x + room.w || y == room.y + room.h;
            maze.setCell(x, y, border ? CellType::Wall : CellType::Floor);
        }
    }
}

void carveFloor(Maze& maze, int x, int y) {
    if (maze.inBounds(x, y) && x > 0 && y > 0 && x < maze.width() - 1 && y < maze.height() - 1) {
        maze.setCell(x, y, CellType::Floor);
    }
}

void drawHorizontalHallway(Maze& maze, int x1, int x2, int y) {
    const int minX = std::min(x1, x2);
    const int maxX = std::max(x1, x2);
    for (int x = minX; x <= maxX; ++x) {
        carveFloor(maze, x, y);
    }
}

void drawVerticalHallway(Maze& maze, int y1, int y2, int x) {
    const int minY = std::min(y1, y2);
    const int maxY = std::max(y1, y2);
    for (int y = minY; y <= maxY; ++y) {
        carveFloor(maze, x, y);
    }
}

void connectRooms(Maze& maze, const Room& a, const Room& b, std::mt19937& rng) {
    const int ax = a.centerX();
    const int ay = a.centerY();
    const int bx = b.centerX();
    const int by = b.centerY();

    if (std::uniform_int_distribution<int>(0, 1)(rng) == 0) {
        drawHorizontalHallway(maze, ax, bx, ay);
        drawVerticalHallway(maze, ay, by, bx);
    } else {
        drawVerticalHallway(maze, ay, by, ax);
        drawHorizontalHallway(maze, ax, bx, by);
    }
}

int roomDistance(const Room& a, const Room& b) {
    return std::abs(a.centerX() - b.centerX()) + std::abs(a.centerY() - b.centerY());
}

void primConnect(Maze& maze, const std::vector<Room>& rooms, std::mt19937& rng) {
    if (rooms.empty()) {
        return;
    }

    const int n = static_cast<int>(rooms.size());
    std::vector<char> inMst(static_cast<size_t>(n), 0);
    std::vector<int> minDist(static_cast<size_t>(n), std::numeric_limits<int>::max());
    std::vector<int> parent(static_cast<size_t>(n), -1);
    minDist[0] = 0;

    for (int i = 0; i < n; ++i) {
        int best = -1;
        for (int v = 0; v < n; ++v) {
            if (!inMst[static_cast<size_t>(v)] &&
                (best == -1 || minDist[static_cast<size_t>(v)] < minDist[static_cast<size_t>(best)])) {
                best = v;
            }
        }
        if (best < 0) {
            break;
        }

        inMst[static_cast<size_t>(best)] = 1;
        if (parent[static_cast<size_t>(best)] >= 0) {
            connectRooms(maze, rooms[static_cast<size_t>(parent[static_cast<size_t>(best)])],
                         rooms[static_cast<size_t>(best)], rng);
        }

        for (int v = 0; v < n; ++v) {
            if (inMst[static_cast<size_t>(v)]) {
                continue;
            }
            const int dist = roomDistance(rooms[static_cast<size_t>(best)], rooms[static_cast<size_t>(v)]);
            if (dist < minDist[static_cast<size_t>(v)]) {
                minDist[static_cast<size_t>(v)] = dist;
                parent[static_cast<size_t>(v)] = best;
            }
        }
    }
}

std::vector<Room> placeRooms(Maze& maze, std::mt19937& rng, int spaceThreshold) {
    std::vector<Room> rooms;
    std::vector<char> claimed(static_cast<size_t>(maze.width() * maze.height()), 0);
    int spaceOccupied = 0;
    int attempts = 0;

    while (spaceOccupied < spaceThreshold && attempts < kMaxPlacementAttempts) {
        ++attempts;
        Room room;
        room.w = std::uniform_int_distribution<int>(kMinRoomWidth, kMinRoomWidth + kRoomWidthVariability - 1)(rng);
        room.h = std::uniform_int_distribution<int>(kMinRoomHeight, kMinRoomHeight + kRoomHeightVariability - 1)(rng);
        room.x = std::uniform_int_distribution<int>(1, std::max(1, maze.width() - room.w - 2))(rng);
        room.y = std::uniform_int_distribution<int>(1, std::max(1, maze.height() - room.h - 2))(rng);

        if (!roomFits(maze, claimed, room)) {
            continue;
        }

        fillRoom(maze, room);
        claimRoom(claimed, maze.width(), room);
        spaceOccupied += room.w * room.h;
        rooms.push_back(room);
    }

    return rooms;
}

GridPosition chooseStart(const Maze& maze) {
    GridPosition best{0, 0};
    int bestScore = std::numeric_limits<int>::max();
    bool found = false;
    for (int y = 0; y < maze.height(); ++y) {
        for (int x = 0; x < maze.width(); ++x) {
            if (!maze.isWalkable(x, y)) {
                continue;
            }
            const int score = x + y;
            if (!found || score < bestScore) {
                best = GridPosition{x, y};
                bestScore = score;
                found = true;
            }
        }
    }
    return best;
}

Maze carveRoomDungeon(int width, int height, uint32_t seed) {
    Maze maze(width, height);
    if (width < 8 || height < 8) {
        return maze;
    }

    std::mt19937 rng(seed);
    const int spacePercent =
        std::uniform_int_distribution<int>(kMinSpacePercent, kMinSpacePercent + kSpacePercentVariability - 1)(rng);
    const int spaceThreshold = (width * height * spacePercent) / 100;

    std::vector<Room> rooms = placeRooms(maze, rng, spaceThreshold);
    if (rooms.empty()) {
        // Fallback: carve a single central room so the maze is never empty.
        Room room{width / 4, height / 4, width / 2, height / 2};
        fillRoom(maze, room);
        rooms.push_back(room);
    }

    primConnect(maze, rooms, rng);

    const GridPosition start = chooseStart(maze);
    maze.setStart(start);
    maze.setExit(maze.farthestFrom(start));
    return maze;
}

}  // namespace

Maze MazeGenerator::generate(int width, int height, uint32_t seed) {
    for (int attempt = 0; attempt < 32; ++attempt) {
        const uint32_t trySeed = seed + static_cast<uint32_t>(attempt);
        Maze maze = carveRoomDungeon(width, height, trySeed);
        if (maze.isWalkable(maze.start()) && maze.isWalkable(maze.exitPosition()) &&
            isSolvable(maze, maze.start(), maze.exitPosition())) {
            return maze;
        }
    }
    return carveRoomDungeon(width, height, seed);
}

bool MazeGenerator::isSolvable(const Maze& maze, GridPosition start, GridPosition end) {
    return maze.bfsDistance(start, end).has_value();
}
