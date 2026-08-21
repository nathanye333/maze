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

void MockGod::beginEvaluate(
    const WorldState& world, std::string_view trigger, std::string_view playerMessage) {
    pending_ = decide(world, trigger, playerMessage);
}

bool MockGod::isBusy() const {
    return false;
}

bool MockGod::tryTakeDecision(GodDecision& out) {
    if (!pending_) {
        return false;
    }
    out = *pending_;
    pending_.reset();
    return true;
}

GodDecision MockGod::decide(
    const WorldState& world, std::string_view trigger, std::string_view playerMessage) {
    GodDecision decision;
    decision.action = GodActionType::None;

    if (trigger == "shrine") {
        decision.action = GodActionType::SendMessage;
        if (!playerMessage.empty()) {
            decision.message = "Your words reach me.";
            decision.favorDelta = 1;
        } else {
            decision.message = kMessages[world.profile.godInteractions % 4];
        }
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

    return decision;
}
