#include "Player.h"

bool tryMovePlayer(Player& player, const Maze& maze, int dx, int dy) {
    const GridPosition next{player.position.x + dx, player.position.y + dy};
    if (!maze.isWalkable(next)) {
        return false;
    }
    player.position = next;
    return true;
}
