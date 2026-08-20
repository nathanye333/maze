#include "WorldState.h"

#include "EventSystem.h"
#include "MazeGenerator.h"

#include <algorithm>
#include <random>

void populateEntities(WorldState& world, uint32_t seed) {
    std::mt19937 rng(seed ^ 0x9E3779B9u);
    auto spots = world.maze.reachableFloors(world.maze.start());
    spots.erase(
        std::remove_if(
            spots.begin(),
            spots.end(),
            [&](GridPosition p) { return p == world.maze.start() || p == world.maze.exitPosition(); }),
        spots.end());
    std::shuffle(spots.begin(), spots.end(), rng);

    world.shrines.clear();
    world.enemies.clear();
    world.nextEnemyId = 1;

    size_t index = 0;
    for (int i = 0; i < SHRINE_COUNT && index < spots.size(); ++i, ++index) {
        world.shrines.push_back(Shrine{spots[index], false, false});
    }
    for (int i = 0; i < INITIAL_ENEMY_COUNT && index < spots.size(); ++i, ++index) {
        Enemy enemy;
        enemy.id = world.nextEnemyId++;
        enemy.position = spots[index];
        enemy.active = true;
        world.enemies.push_back(enemy);
    }
}

void rebuildMaze(WorldState& world, MazeGenerator& gen, uint32_t seed, bool keepHealth) {
    const int health = keepHealth ? world.player.health : PLAYER_MAX_HEALTH;
    const PlayerProfile profile = world.profile;
    const int favor = world.godFavor;
    const int power = world.godPower;
    const int level = world.currentLevel;
    const auto recent = world.recentEvents;

    world.maze = gen.generate(MAZE_WIDTH, MAZE_HEIGHT, seed);
    world.mazeSeed = seed;
    world.player.position = world.maze.start();
    world.player.health = health;
    world.player.moveCooldown = 0.0f;
    world.godFavor = favor;
    world.godPower = power;
    world.currentLevel = level;
    world.profile = profile;
    world.recentEvents = recent;
    populateEntities(world, seed);
}

void initializeWorld(WorldState& world, MazeGenerator& gen, EventSystem& events, uint32_t seed, int gameTime) {
    world = WorldState{};
    world.currentLevel = 1;
    world.godFavor = 0;
    world.godPower = 10;
    rebuildMaze(world, gen, seed, false);
    events.record(EventType::MazeStarted, gameTime, "A new maze began.");
    syncRecentEvents(world, events);
}

void restartCurrentMaze(WorldState& world, MazeGenerator& gen, EventSystem& events, int gameTime) {
    (void)events;
    (void)gameTime;
    rebuildMaze(world, gen, world.mazeSeed, false);
    world.player.health = PLAYER_MAX_HEALTH;
}

void syncRecentEvents(WorldState& world, const EventSystem& events) {
    world.recentEvents = events.recent();
}

nlohmann::json toJson(const WorldState& world) {
    nlohmann::json profile = {
        {"enemies_killed", world.profile.enemiesKilled},
        {"damage_taken", world.profile.damageTaken},
        {"shrines_visited", world.profile.shrinesVisited},
        {"god_interactions", world.profile.godInteractions},
    };

    const auto distance = world.maze.bfsDistance(world.player.position, world.maze.exitPosition());

    nlohmann::json events = nlohmann::json::array();
    for (const GameEvent& event : world.recentEvents) {
        events.push_back(event.description);
    }

    return {
        {"player",
         {{"health", world.player.health},
          {"position", {world.player.position.x, world.player.position.y}}}},
        {"profile", profile},
        {"maze",
         {{"width", world.maze.width()},
          {"height", world.maze.height()},
          {"distance_to_exit", distance.value_or(-1)}}},
        {"god", {{"favor", world.godFavor}, {"power", world.godPower}}},
        {"recent_events", events},
    };
}
