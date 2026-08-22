#pragma once

#include "CombatAnimation.h"
#include "Enemy.h"
#include "Event.h"
#include "Maze.h"
#include "Player.h"
#include "Projectile.h"
#include "Shrine.h"
#include "Weapon.h"
#include "WeaponPickup.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct PlayerProfile {
    int enemiesKilled = 0;
    int damageTaken = 0;
    int deaths = 0;
    int shrinesVisited = 0;
    int godInteractions = 0;
    int mazeRegenerationsExperienced = 0;
    int distanceTravelled = 0;
};

struct WorldState {
    Maze maze;
    Player player;
    std::vector<Enemy> enemies;
    std::vector<Shrine> shrines;

    int currentLevel = 1;
    uint32_t mazeSeed = 1;

    std::vector<GameEvent> recentEvents;

    int godFavor = 0;
    int godPower = 10;

    PlayerProfile profile;
    int nextEnemyId = 1;

    PlayerInventory inventory;
    std::vector<WeaponPickup> weaponPickups;
    std::vector<Projectile> projectiles;
    CombatVisualState combatVisuals;
    int nextProjectileId = 1;
};

class MazeGenerator;
class EventSystem;

void populateEntities(WorldState& world, uint32_t seed);
void initializeWorld(WorldState& world, MazeGenerator& gen, EventSystem& events, uint32_t seed, int gameTime);
void restartCurrentMaze(WorldState& world, MazeGenerator& gen, EventSystem& events, int gameTime);
void rebuildMaze(WorldState& world, MazeGenerator& gen, uint32_t seed, bool keepHealth);
void syncRecentEvents(WorldState& world, const EventSystem& events);
nlohmann::json toJson(
    const WorldState& world,
    std::string_view trigger = "ambient",
    std::string_view playerMessage = {});
