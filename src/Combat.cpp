#include "Combat.h"

#include "Enemy.h"
#include "Projectile.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace {

GridPosition stepFromFacing(GridPosition origin, GridPosition facing) {
    return GridPosition{origin.x + facing.x, origin.y + facing.y};
}

GridPosition normalizeFacing(GridPosition facing) {
    int x = facing.x;
    int y = facing.y;
    if (x != 0) {
        x = x > 0 ? 1 : -1;
    }
    if (y != 0) {
        y = y > 0 ? 1 : -1;
    }
    if (x == 0 && y == 0) {
        return GridPosition{0, 1};
    }
    if (x != 0 && y != 0) {
        if (std::abs(x) >= std::abs(y)) {
            y = 0;
        } else {
            x = 0;
        }
    }
    return GridPosition{x, y};
}

Enemy* findEnemyAt(std::vector<Enemy>& enemies, GridPosition position) {
    for (Enemy& enemy : enemies) {
        if (enemy.active && enemy.position == position) {
            return &enemy;
        }
    }
    return nullptr;
}

bool fireGun(
    WorldState& world,
    EventSystem& events,
    int gameTime,
    CombatVisualState& visuals,
    const WeaponDef& weapon,
    GridPosition facing) {
    const GridPosition muzzle = stepFromFacing(world.player.position, facing);
    if (!world.maze.inBounds(muzzle) || !world.maze.isWalkable(muzzle)) {
        return false;
    }
    if (!hasLineOfSight(world.maze, world.player.position, muzzle)) {
        return false;
    }

    spawnProjectile(
        world.projectiles,
        world.nextProjectileId,
        world.player.position,
        facing,
        weapon.damage,
        weapon.projectileSpeed,
        weapon.range);
    spawnCombatAnimation(visuals, CombatAnimKind::GunMuzzleFlash, world.player.position, facing);

    std::ostringstream desc;
    desc << "Player fired " << weapon.name << " toward (" << facing.x << ", " << facing.y << ").";
    events.record(EventType::PlayerAttacked, gameTime, desc.str());
    syncRecentEvents(world, events);
    return true;
}

bool swingSword(
    WorldState& world,
    EventSystem& events,
    int gameTime,
    CombatVisualState& visuals,
    const WeaponDef& weapon,
    GridPosition facing) {
    const GridPosition targetCell = stepFromFacing(world.player.position, facing);
    spawnCombatAnimation(visuals, CombatAnimKind::SwordSwing, world.player.position, facing);

    if (Enemy* enemy = findEnemyAt(world.enemies, targetCell)) {
        if (manhattan(enemy->position, world.player.position) <= weapon.range) {
            applyDamageToEnemy(*enemy, weapon.damage, world, events, gameTime, visuals);
        }
    }

    std::ostringstream desc;
    desc << "Player attacked with " << weapon.name << ".";
    events.record(EventType::PlayerAttacked, gameTime, desc.str());
    syncRecentEvents(world, events);
    return true;
}

}  // namespace

GridPosition computeAimFacing(GridPosition player, float aimWorldX, float aimWorldY, int cellSize) {
    const float playerCenterX = player.x * static_cast<float>(cellSize) + cellSize * 0.5f;
    const float playerCenterY = player.y * static_cast<float>(cellSize) + cellSize * 0.5f;
    float dx = aimWorldX - playerCenterX;
    float dy = aimWorldY - playerCenterY;

    if (std::abs(dx) < 0.001f && std::abs(dy) < 0.001f) {
        return GridPosition{0, 1};
    }

    int facingX = 0;
    int facingY = 0;
    if (std::abs(dx) >= std::abs(dy)) {
        facingX = dx > 0.0f ? 1 : -1;
    } else {
        facingY = dy > 0.0f ? 1 : -1;
    }
    return GridPosition{facingX, facingY};
}

bool hasLineOfSight(const Maze& maze, GridPosition from, GridPosition to) {
    if (!maze.inBounds(from) || !maze.inBounds(to)) {
        return false;
    }

    int currX = from.x;
    int currY = from.y;
    const int targetX = to.x;
    const int targetY = to.y;
    const int dx = std::abs(targetX - from.x);
    const int dy = std::abs(targetY - from.y);
    int error = dx - dy;
    const int xInc = targetX < from.x ? -1 : 1;
    const int yInc = targetY < from.y ? -1 : 1;

    while (true) {
        if (currX == targetX && currY == targetY) {
            return true;
        }

        const int error2 = 2 * error;
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
            return false;
        }

        if (!maze.inBounds(currX, currY)) {
            return false;
        }
        if (maze.cellAt(currX, currY) == CellType::Wall) {
            return false;
        }
    }
}

bool applyDamageToEnemy(
    Enemy& enemy,
    int damage,
    WorldState& world,
    EventSystem& events,
    int gameTime,
    CombatVisualState& visuals) {
    if (!enemy.active || damage <= 0) {
        return false;
    }

    enemy.health -= damage;
    spawnCombatAnimation(visuals, CombatAnimKind::HitFlash, enemy.position, {});

    if (enemy.health > 0) {
        return true;
    }

    enemy.health = 0;
    enemy.active = false;
    world.profile.enemiesKilled += 1;
    std::ostringstream desc;
    desc << "Player killed an enemy at (" << enemy.position.x << ", " << enemy.position.y << ").";
    events.record(EventType::EnemyKilled, gameTime, desc.str());
    syncRecentEvents(world, events);
    return true;
}

bool tryPlayerAttack(WorldState& world, EventSystem& events, int gameTime, CombatVisualState& visuals) {
    const WeaponDef* weapon = equippedWeapon(world.inventory);
    if (weapon == nullptr || world.inventory.attackCooldown > 0.0f) {
        return false;
    }

    const GridPosition facing = normalizeFacing(world.inventory.facing);
    bool attacked = false;
    if (weapon->kind == WeaponKind::Melee) {
        attacked = swingSword(world, events, gameTime, visuals, *weapon, facing);
    } else {
        attacked = fireGun(world, events, gameTime, visuals, *weapon, facing);
    }

    if (attacked) {
        world.inventory.attackCooldown = weapon->cooldown;
    }
    return attacked;
}

void updateCombat(WorldState& world, EventSystem& events, int gameTime, float dt, CombatVisualState& visuals) {
    tickPlayerInventory(world.inventory, dt);
    updateProjectiles(world.projectiles, world.maze, world.enemies, gameTime, dt, events, world, visuals);
    updateCombatAnimations(visuals, dt);
}
