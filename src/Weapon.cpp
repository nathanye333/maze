#include "Weapon.h"

#include <algorithm>
#include <stdexcept>

namespace {

const WeaponDef kSword{
    "sword",
    "Sword",
    WeaponKind::Melee,
    35,
    1,
    0.35f,
    0.0f};

const WeaponDef kGun{
    "gun",
    "Gun",
    WeaponKind::Ranged,
    20,
    10,
    0.45f,
    12.0f};

const WeaponDef* lookupWeapon(std::string_view id) {
    if (id == kSword.id) {
        return &kSword;
    }
    if (id == kGun.id) {
        return &kGun;
    }
    return nullptr;
}

}  // namespace

const WeaponDef& weaponDefById(std::string_view id) {
    if (const WeaponDef* weapon = lookupWeapon(id)) {
        return *weapon;
    }
    throw std::invalid_argument("unknown weapon id");
}

bool isRegisteredWeapon(std::string_view id) {
    return lookupWeapon(id) != nullptr;
}

const WeaponDef* equippedWeapon(const PlayerInventory& inventory) {
    if (inventory.equippedIndex < 0 ||
        inventory.equippedIndex >= static_cast<int>(inventory.weaponIds.size())) {
        return nullptr;
    }
    return &weaponDefById(inventory.weaponIds[static_cast<size_t>(inventory.equippedIndex)]);
}

bool ownsWeapon(const PlayerInventory& inventory, std::string_view weaponId) {
    return std::find(inventory.weaponIds.begin(), inventory.weaponIds.end(), weaponId) !=
           inventory.weaponIds.end();
}

bool addWeaponToInventory(PlayerInventory& inventory, std::string_view weaponId) {
    if (!isRegisteredWeapon(weaponId) || ownsWeapon(inventory, weaponId)) {
        return false;
    }
    inventory.weaponIds.emplace_back(weaponId);
    inventory.equippedIndex = static_cast<int>(inventory.weaponIds.size()) - 1;
    return true;
}

bool dropEquippedWeapon(PlayerInventory& inventory, std::string& outWeaponId) {
    if (inventory.equippedIndex < 0 ||
        inventory.equippedIndex >= static_cast<int>(inventory.weaponIds.size())) {
        return false;
    }
    outWeaponId = inventory.weaponIds[static_cast<size_t>(inventory.equippedIndex)];
    inventory.weaponIds.erase(inventory.weaponIds.begin() + inventory.equippedIndex);
    if (inventory.weaponIds.empty()) {
        inventory.equippedIndex = -1;
    } else if (inventory.equippedIndex >= static_cast<int>(inventory.weaponIds.size())) {
        inventory.equippedIndex = static_cast<int>(inventory.weaponIds.size()) - 1;
    }
    return true;
}

void equipWeaponIndex(PlayerInventory& inventory, int index) {
    if (index >= 0 && index < static_cast<int>(inventory.weaponIds.size())) {
        inventory.equippedIndex = index;
    }
}

void cycleEquippedWeapon(PlayerInventory& inventory) {
    if (inventory.weaponIds.empty()) {
        return;
    }
    if (inventory.equippedIndex < 0) {
        inventory.equippedIndex = 0;
        return;
    }
    inventory.equippedIndex =
        (inventory.equippedIndex + 1) % static_cast<int>(inventory.weaponIds.size());
}

void tickPlayerInventory(PlayerInventory& inventory, float dt) {
    if (inventory.attackCooldown > 0.0f) {
        inventory.attackCooldown -= dt;
        if (inventory.attackCooldown < 0.0f) {
            inventory.attackCooldown = 0.0f;
        }
    }
}
