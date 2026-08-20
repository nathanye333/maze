#include "Enemy.h"

int activeEnemyCount(const std::vector<Enemy>& enemies) {
    int count = 0;
    for (const Enemy& enemy : enemies) {
        if (enemy.active) {
            ++count;
        }
    }
    return count;
}

GridPosition chooseEnemyMove(const Enemy& enemy, const Maze& maze, GridPosition playerPos, std::mt19937& rng) {
    if (manhattan(enemy.position, playerPos) <= ENEMY_DETECTION_RANGE) {
        if (auto step = maze.nextStepToward(enemy.position, playerPos)) {
            return *step;
        }
    }

    auto neighbors = maze.walkableNeighbors(enemy.position);
    if (neighbors.empty()) {
        return enemy.position;
    }
    std::uniform_int_distribution<size_t> pick(0, neighbors.size() - 1);
    return neighbors[pick(rng)];
}
