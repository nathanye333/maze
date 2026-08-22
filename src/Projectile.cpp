#include "Projectile.h"

#include "Combat.h"
#include "Enemy.h"
#include "EventSystem.h"
#include "WorldState.h"

#include <algorithm>

namespace {

Enemy* findEnemyAt(std::vector<Enemy>& enemies, GridPosition position) {
    for (Enemy& enemy : enemies) {
        if (enemy.active && enemy.position == position) {
            return &enemy;
        }
    }
    return nullptr;
}

Projectile* allocateProjectile(std::vector<Projectile>& projectiles, int& nextProjectileId) {
    for (Projectile& projectile : projectiles) {
        if (!projectile.active) {
            projectile = Projectile{};
            projectile.id = nextProjectileId++;
            projectile.active = true;
            return &projectile;
        }
    }
    if (static_cast<int>(projectiles.size()) >= MAX_PROJECTILES) {
        return nullptr;
    }
    Projectile projectile;
    projectile.id = nextProjectileId++;
    projectile.active = true;
    projectiles.push_back(projectile);
    return &projectiles.back();
}

GridPosition stepCell(GridPosition cell, GridPosition direction) {
    return GridPosition{cell.x + direction.x, cell.y + direction.y};
}

}  // namespace

void spawnProjectile(
    std::vector<Projectile>& projectiles,
    int& nextProjectileId,
    GridPosition origin,
    GridPosition direction,
    int damage,
    float speed,
    int maxRange) {
    Projectile* projectile = allocateProjectile(projectiles, nextProjectileId);
    if (projectile == nullptr) {
        return;
    }

    projectile->cell = origin;
    projectile->direction = direction;
    projectile->damage = damage;
    projectile->distanceTravelled = 0;
    projectile->maxRange = maxRange;
    projectile->active = true;

    if (speed <= 0.0f) {
        speed = 1.0f;
    }
    projectile->speed = speed;
    projectile->moveTimer = 1.0f / speed;
}

void updateProjectiles(
    std::vector<Projectile>& projectiles,
    const Maze& maze,
    std::vector<Enemy>& enemies,
    int gameTime,
    float dt,
    EventSystem& events,
    WorldState& world,
    CombatVisualState& visuals) {
    for (Projectile& projectile : projectiles) {
        if (!projectile.active) {
            continue;
        }

        projectile.moveTimer -= dt;
        if (projectile.moveTimer > 0.0f) {
            continue;
        }

        projectile.moveTimer += 1.0f / projectile.speed;

        const GridPosition next = stepCell(projectile.cell, projectile.direction);
        if (!maze.inBounds(next) || !maze.isWalkable(next)) {
            projectile.active = false;
            continue;
        }

        projectile.cell = next;
        projectile.distanceTravelled += 1;
        if (projectile.distanceTravelled > projectile.maxRange) {
            projectile.active = false;
            continue;
        }

        if (Enemy* enemy = findEnemyAt(enemies, next)) {
            applyDamageToEnemy(*enemy, projectile.damage, world, events, gameTime, visuals);
            projectile.active = false;
        }
    }
}
