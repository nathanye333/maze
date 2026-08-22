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

    CHECK(isRegisteredWeapon("sword"), "sword is registered");
    CHECK(isRegisteredWeapon("gun"), "gun is registered");
    CHECK(world.weaponPickups.size() == 2, "maze spawns sword and gun pickups");
    CHECK(!isWeaponPickupOnShrine(world.weaponPickups, world.shrines), "initial weapon spawns avoid shrines");

    Enemy& enemy = world.enemies.front();
    enemy.active = true;
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.position = GridPosition{world.player.position.x + 1, world.player.position.y};
    world.inventory.facing = GridPosition{1, 0};
    addWeaponToInventory(world.inventory, "sword");

    const size_t animBefore = world.combatVisuals.active.size();
    CHECK(tryPlayerAttack(world, events, 1, world.combatVisuals), "sword attack hits enemy");
    CHECK(enemy.health < ENEMY_MAX_HEALTH, "sword reduces enemy health");
    CHECK(world.combatVisuals.active.size() > animBefore, "sword attack spawns animation");

    world.inventory.attackCooldown = 0.0f;
    enemy.health = 1;
    CHECK(tryPlayerAttack(world, events, 2, world.combatVisuals), "sword attack finishes enemy");
    CHECK(!enemy.active, "enemy deactivated at zero health");

    world.inventory.attackCooldown = 0.0f;
    addWeaponToInventory(world.inventory, "sword");
    enemy.active = true;
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.position = GridPosition{world.player.position.x - 1, world.player.position.y};
    world.inventory.facing = GridPosition{1, 0};
    const int healthBeforeMiss = enemy.health;
    CHECK(tryPlayerAttack(world, events, 3, world.combatVisuals), "sword swing in facing direction");
    CHECK(enemy.health == healthBeforeMiss, "sword does not hit enemy behind facing direction");

    world.inventory.attackCooldown = 0.0f;
    enemy.position = GridPosition{world.player.position.x + 1, world.player.position.y};
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.active = true;
    CHECK(tryPlayerAttack(world, events, 4, world.combatVisuals), "sword attack succeeds");
    const int healthAfterFirst = enemy.health;
    CHECK(!tryPlayerAttack(world, events, 5, world.combatVisuals), "sword respects cooldown");
    CHECK(enemy.health == healthAfterFirst, "cooldown prevents extra sword damage");

    world.projectiles.clear();
    addWeaponToInventory(world.inventory, "gun");
    equipWeaponIndex(world.inventory, 1);
    world.inventory.attackCooldown = 0.0f;
    world.inventory.facing = GridPosition{1, 0};
    enemy.active = true;
    enemy.health = ENEMY_MAX_HEALTH;
    enemy.position = GridPosition{world.player.position.x + 3, world.player.position.y};

    CHECK(tryPlayerAttack(world, events, 6, world.combatVisuals), "gun fires projectile");
    CHECK(!world.projectiles.empty(), "gun creates projectile");

    for (int step = 0; step < 20; ++step) {
        updateProjectiles(
            world.projectiles,
            world.maze,
            world.enemies,
            6 + step,
            0.2f,
            events,
            world,
            world.combatVisuals);
    }
    CHECK(enemy.health < ENEMY_MAX_HEALTH, "gun projectile damages enemy");

    Projectile wallTestProjectile;
    wallTestProjectile.active = true;
    wallTestProjectile.cell = world.player.position;
    wallTestProjectile.direction = GridPosition{0, -1};
    wallTestProjectile.damage = 10;
    wallTestProjectile.speed = 12.0f;
    wallTestProjectile.maxRange = 10;
    wallTestProjectile.moveTimer = 0.0f;
    world.projectiles.clear();
    world.projectiles.push_back(wallTestProjectile);
    while (world.projectiles.front().active) {
        updateProjectiles(
            world.projectiles,
            world.maze,
            world.enemies,
            20,
            0.2f,
            events,
            world,
            world.combatVisuals);
    }
    CHECK(!world.projectiles.front().active, "projectile stops at wall or max range");

    const GridPosition losFrom = world.player.position;
    GridPosition losTo = losFrom;
    losTo.x += 1;
    while (world.maze.isWalkable(losTo)) {
        losTo.x += 1;
    }
    losTo.x -= 1;
    if (losTo != losFrom) {
        CHECK(hasLineOfSight(world.maze, losFrom, losTo), "line of sight is clear along open hallway");
        GridPosition blocked = losTo;
        blocked.x += 1;
        if (world.maze.inBounds(blocked) && !world.maze.isWalkable(blocked)) {
            CHECK(!hasLineOfSight(world.maze, losFrom, blocked), "line of sight blocked by wall");
        }
    }

    WeaponPickup pickup;
    pickup.weaponId = "sword";
    pickup.position = world.player.position;
    pickup.collected = false;
    world.weaponPickups.clear();
    world.inventory = PlayerInventory{};
    world.weaponPickups.push_back(pickup);
    CHECK(tryCollectWeaponPickups(world, events, 30), "E pickup collects weapon on same tile");
    CHECK(ownsWeapon(world.inventory, "sword"), "inventory contains collected weapon");

    addWeaponToInventory(world.inventory, "gun");
    equipWeaponIndex(world.inventory, 0);
    CHECK(equippedWeapon(world.inventory)->id == "sword", "equip selects sword");
    cycleEquippedWeapon(world.inventory);
    CHECK(equippedWeapon(world.inventory)->id == "gun", "cycle equips next weapon");

    world.inventory = PlayerInventory{};
    CHECK(!tryPlayerAttack(world, events, 31, world.combatVisuals), "unarmed player cannot attack");

    const GridPosition shrinePos = world.shrines.front().position;
    world.player.position = shrinePos;
    world.inventory = PlayerInventory{};
    world.weaponPickups.clear();
    world.weaponPickups.push_back(WeaponPickup{shrinePos, "sword", false});
    CHECK(tryCollectWeaponPickups(world, events, 40), "pickup works when weapon is on shrine tile");
    CHECK(ownsWeapon(world.inventory, "sword"), "shrine tile pickup enters inventory");

    world.player.position = world.maze.start();
    world.inventory = PlayerInventory{};
    addWeaponToInventory(world.inventory, "gun");
    CHECK(tryDropEquippedWeapon(world, events, 41), "G drops equipped weapon");
    CHECK(findWeaponPickupAt(world.weaponPickups, world.player.position) != nullptr, "dropped weapon is on ground");
    CHECK(!ownsWeapon(world.inventory, "gun"), "dropped weapon leaves inventory");

    addWeaponToInventory(world.inventory, "sword");
    CHECK(!tryDropEquippedWeapon(world, events, 42), "cannot drop onto occupied pickup tile");
}
