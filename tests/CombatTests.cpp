#include "TestSupport.h"

#include "Combat.h"
#include "CombatAnimation.h"
#include "EventSystem.h"
#include "MazeGenerator.h"
#include "Weapon.h"
#include "WeaponPickup.h"
#include "WorldState.h"

void runCombatTests(int& passed, int& failed) {
    EventSystem events;
    MazeGenerator gen;
    WorldState world;
    initializeWorld(world, gen, events, 42, 0);

    CHECK((computeAimFacing({5, 5}, 5 * 16 + 8, 4 * 16 + 8, 16) == GridPosition{0, -1}), "aim facing north");
    CHECK((computeAimFacing({5, 5}, 6 * 16 + 8, 5 * 16 + 8, 16) == GridPosition{1, 0}), "aim facing east");

    Enemy& enemy = world.enemies.front();
    enemy.active = true;
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.position = GridPosition{world.player.position.x + 1, world.player.position.y};
    world.inventory.facing = GridPosition{1, 0};
    addWeaponToInventory(world.inventory, "sword");

    CHECK(tryPlayerAttack(world, events, 1, world.combatVisuals), "sword attack hits enemy");
    CHECK(enemy.health < ENEMY_MAX_HEALTH, "sword reduces enemy health");

    world.inventory.attackCooldown = 0.0f;
    enemy.health = 1;
    CHECK(tryPlayerAttack(world, events, 2, world.combatVisuals), "sword attack finishes enemy");
    CHECK(!enemy.active, "enemy deactivated at zero health");

    world.projectiles.clear();
    addWeaponToInventory(world.inventory, "gun");
    equipWeaponIndex(world.inventory, 1);
    world.inventory.attackCooldown = 0.0f;
    world.inventory.facing = GridPosition{1, 0};
    enemy.active = true;
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.position = GridPosition{world.player.position.x + 3, world.player.position.y};

    CHECK(tryPlayerAttack(world, events, 3, world.combatVisuals), "gun fires projectile");
    CHECK(!world.projectiles.empty(), "gun creates projectile");

    for (int step = 0; step < 20; ++step) {
        updateProjectiles(
            world.projectiles,
            world.maze,
            world.enemies,
            3 + step,
            0.2f,
            events,
            world,
            world.combatVisuals);
    }
    CHECK(enemy.health < ENEMY_MAX_HEALTH, "gun projectile damages enemy");

    WeaponPickup pickup;
    pickup.weaponId = "sword";
    pickup.position = world.player.position;
    pickup.collected = false;
    world.weaponPickups.clear();
    world.inventory = PlayerInventory{};
    world.weaponPickups.push_back(pickup);
    CHECK(tryCollectWeaponPickups(world, events, 10), "walking over pickup collects weapon");
    CHECK(ownsWeapon(world.inventory, "sword"), "inventory contains collected weapon");

    world.inventory = PlayerInventory{};
    CHECK(!tryPlayerAttack(world, events, 11, world.combatVisuals), "unarmed player cannot attack");
}
