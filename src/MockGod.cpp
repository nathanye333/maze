#include "MockGod.h"

#include "Maze.h"

namespace {

const char* kMessages[] = {
    "You are being watched.",
    "I wonder why you continue.",
    "Keep going.",
    "Do you really believe the exit is real?",
};

int activeCount(const WorldState& world) {
    return activeEnemyCount(world.enemies);
}

int distanceToExit(const WorldState& world) {
    if (auto d = world.maze.bfsDistance(world.player.position, world.maze.exitPosition())) {
        return *d;
    }
    return manhattan(world.player.position, world.maze.exitPosition());
}

}  // namespace

GodDecision MockGod::evaluate(const WorldState& world) {
    GodDecision decision;
    decision.action = GodActionType::None;

    if (world.profile.godInteractions > lastGodInteractions_) {
        lastGodInteractions_ = world.profile.godInteractions;
        decision.action = GodActionType::SendMessage;
        decision.message = kMessages[world.profile.godInteractions % 4];
        decision.powerCost = expectedPowerCost(decision.action);
        return decision;
    }

    if (distanceToExit(world) <= 6 && world.godPower >= COST_REGENERATE && lastRegenSeed_ != world.mazeSeed) {
        lastRegenSeed_ = world.mazeSeed;
        decision.action = GodActionType::RegenerateMaze;
        decision.powerCost = expectedPowerCost(decision.action);
        return decision;
    }

    const int damageBand = world.profile.damageTaken / 30;
    if (world.profile.damageTaken >= 30 && damageBand > lastMessageDamageBand_) {
        lastMessageDamageBand_ = damageBand;
        decision.action = GodActionType::SendMessage;
        decision.message = "You bleed, and still you walk.";
        decision.powerCost = expectedPowerCost(decision.action);
        return decision;
    }

    if (world.profile.enemiesKilled >= 2 && activeCount(world) < MAX_ENEMIES &&
        world.profile.enemiesKilled != lastSpawnKillCount_) {
        lastSpawnKillCount_ = world.profile.enemiesKilled;
        decision.action = GodActionType::SpawnEnemy;
        decision.parameter = "near_player";
        decision.powerCost = expectedPowerCost(decision.action);
        return decision;
    }

    if (world.profile.godInteractions >= 1 && lastTeleportInteractions_ < world.profile.godInteractions &&
        world.godPower >= COST_TELEPORT) {
        lastTeleportInteractions_ = world.profile.godInteractions;
        decision.action = GodActionType::TeleportPlayer;
        decision.parameter = "dead_end";
        decision.powerCost = expectedPowerCost(decision.action);
        return decision;
    }

    return decision;
}
