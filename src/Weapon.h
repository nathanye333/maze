#pragma once

#include "Maze.h"

#include <string>
#include <string_view>
#include <vector>

enum class WeaponKind {
    Melee,
    Ranged
};

struct WeaponDef {
    std::string id;
    std::string name;
    WeaponKind kind = WeaponKind::Melee;
    int damage = 0;
    int range = 1;
    float cooldown = 0.0f;
    float projectileSpeed = 0.0f;
};

const WeaponDef& weaponDefById(std::string_view id);
bool isRegisteredWeapon(std::string_view id);

struct PlayerInventory {
    std::vector<std::string> weaponIds;
    int equippedIndex = -1;
    float attackCooldown = 0.0f;
    GridPosition facing{0, 1};
};

const WeaponDef* equippedWeapon(const PlayerInventory& inventory);
bool ownsWeapon(const PlayerInventory& inventory, std::string_view weaponId);
bool addWeaponToInventory(PlayerInventory& inventory, std::string_view weaponId);
void equipWeaponIndex(PlayerInventory& inventory, int index);
void cycleEquippedWeapon(PlayerInventory& inventory);
void tickPlayerInventory(PlayerInventory& inventory, float dt);
