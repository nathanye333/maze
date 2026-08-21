#include "God.h"

#include "EventSystem.h"
#include "MazeGenerator.h"

#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>

int expectedPowerCost(GodActionType action) {
    switch (action) {
        case GodActionType::SendMessage:
            return COST_SEND_MESSAGE;
        case GodActionType::SpawnEnemy:
            return COST_SPAWN_ENEMY;
        case GodActionType::TeleportPlayer:
            return COST_TELEPORT;
        case GodActionType::RegenerateMaze:
            return COST_REGENERATE;
        case GodActionType::None:
            return 0;
    }
    return 0;
}

int addGodPower(int current, int amount) {
    return std::max(0, std::min(MAX_GOD_POWER, current + amount));
}

int addGodFavor(int current, int amount) {
    return std::max(MIN_GOD_FAVOR, std::min(MAX_GOD_FAVOR, current + amount));
}

int clampFavorDelta(int delta) {
    return std::max(-MAX_FAVOR_DELTA, std::min(MAX_FAVOR_DELTA, delta));
}

bool isValidTeleportParameter(const std::string& parameter) {
    return parameter == "random_safe" || parameter == "dead_end" || parameter == "far_from_exit" ||
           parameter == "near_enemy";
}

bool isValidSpawnParameter(const std::string& parameter) {
    return parameter == "random" || parameter == "near_player" || parameter == "far_from_player";
}

namespace {

std::string trimAscii(std::string_view text) {
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

std::string extractJsonObject(std::string_view text) {
    std::string cleaned = trimAscii(text);
    if (cleaned.rfind("```", 0) == 0) {
        const size_t firstNl = cleaned.find('\n');
        if (firstNl != std::string::npos) {
            cleaned = cleaned.substr(firstNl + 1);
        }
        const size_t fence = cleaned.rfind("```");
        if (fence != std::string::npos) {
            cleaned = cleaned.substr(0, fence);
        }
        cleaned = trimAscii(cleaned);
    }
    const size_t start = cleaned.find('{');
    const size_t end = cleaned.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end < start) {
        return {};
    }
    return cleaned.substr(start, end - start + 1);
}

GodActionType parseActionName(std::string_view name) {
    if (name == "none") {
        return GodActionType::None;
    }
    if (name == "regenerate_maze") {
        return GodActionType::RegenerateMaze;
    }
    if (name == "teleport_player") {
        return GodActionType::TeleportPlayer;
    }
    if (name == "spawn_enemy") {
        return GodActionType::SpawnEnemy;
    }
    if (name == "send_message") {
        return GodActionType::SendMessage;
    }
    return GodActionType::None;
}

}  // namespace

bool parseGodDecisionJson(std::string_view text, GodDecision& out) {
    out = GodDecision{};
    const std::string jsonText = extractJsonObject(text);
    if (jsonText.empty()) {
        return false;
    }

    try {
        const nlohmann::json json = nlohmann::json::parse(jsonText);
        if (!json.is_object()) {
            return false;
        }
        const std::string action = json.value("action", "none");
        out.action = parseActionName(action);
        out.parameter = json.value("parameter", "");
        out.message = json.value("message", "");
        out.powerCost = expectedPowerCost(out.action);
        out.favorDelta = clampFavorDelta(json.value("favor_delta", 0));
        return true;
    } catch (...) {
        out = GodDecision{};
        return false;
    }
}

namespace {

bool occupiedByActiveEnemy(const WorldState& world, GridPosition pos, int ignoreId = -1) {
    for (const Enemy& enemy : world.enemies) {
        if (enemy.active && enemy.id != ignoreId && enemy.position == pos) {
            return true;
        }
    }
    return false;
}

std::vector<GridPosition> candidateFloors(const WorldState& world, bool excludePlayer) {
    std::vector<GridPosition> floors;
    for (GridPosition p : world.maze.reachableFloors(world.maze.start())) {
        if (excludePlayer && p == world.player.position) {
            continue;
        }
        floors.push_back(p);
    }
    return floors;
}

std::mt19937 decisionRng(const WorldState& world, int gameTime) {
    return std::mt19937(world.mazeSeed ^ static_cast<uint32_t>(gameTime * 2654435761u) ^
                        static_cast<uint32_t>(world.profile.distanceTravelled));
}

std::optional<GridPosition> pickTeleportTarget(
    const WorldState& world, const std::string& parameter, std::mt19937& rng) {
    auto floors = candidateFloors(world, false);
    if (floors.empty()) {
        return std::nullopt;
    }

    if (parameter == "dead_end") {
        std::vector<GridPosition> deadEnds;
        for (GridPosition p : floors) {
            if (world.maze.walkableNeighborCount(p) == 1) {
                deadEnds.push_back(p);
            }
        }
        if (!deadEnds.empty()) {
            std::uniform_int_distribution<size_t> pick(0, deadEnds.size() - 1);
            return deadEnds[pick(rng)];
        }
    }

    if (parameter == "far_from_exit") {
        GridPosition best = floors.front();
        int bestDist = -1;
        for (GridPosition p : floors) {
            auto dist = world.maze.bfsDistance(p, world.maze.exitPosition());
            if (dist && *dist > bestDist) {
                bestDist = *dist;
                best = p;
            }
        }
        return best;
    }

    if (parameter == "near_enemy") {
        GridPosition best = floors.front();
        int bestDist = 1'000'000;
        bool found = false;
        for (GridPosition p : floors) {
            for (const Enemy& enemy : world.enemies) {
                if (!enemy.active) {
                    continue;
                }
                const int d = manhattan(p, enemy.position);
                if (d < bestDist) {
                    bestDist = d;
                    best = p;
                    found = true;
                }
            }
        }
        if (found) {
            return best;
        }
    }

    std::uniform_int_distribution<size_t> pick(0, floors.size() - 1);
    return floors[pick(rng)];
}

std::optional<GridPosition> pickSpawnTarget(
    const WorldState& world, const std::string& parameter, std::mt19937& rng) {
    auto floors = candidateFloors(world, true);
    floors.erase(
        std::remove_if(
            floors.begin(),
            floors.end(),
            [&](GridPosition p) { return occupiedByActiveEnemy(world, p); }),
        floors.end());
    if (floors.empty()) {
        floors = candidateFloors(world, true);
    }
    if (floors.empty()) {
        return std::nullopt;
    }

    if (parameter == "near_player") {
        GridPosition best = floors.front();
        int bestDist = 1'000'000;
        for (GridPosition p : floors) {
            const int d = manhattan(p, world.player.position);
            if (d >= 1 && d < bestDist) {
                bestDist = d;
                best = p;
            }
        }
        return best;
    }

    if (parameter == "far_from_player") {
        GridPosition best = floors.front();
        int bestDist = -1;
        for (GridPosition p : floors) {
            const int d = manhattan(p, world.player.position);
            if (d > bestDist) {
                bestDist = d;
                best = p;
            }
        }
        return best;
    }

    std::uniform_int_distribution<size_t> pick(0, floors.size() - 1);
    return floors[pick(rng)];
}

bool spawnEnemyAt(WorldState& world, GridPosition pos) {
    if (!world.maze.isWalkable(pos) || pos == world.player.position) {
        return false;
    }
    if (activeEnemyCount(world.enemies) >= MAX_ENEMIES) {
        return false;
    }

    for (Enemy& enemy : world.enemies) {
        if (!enemy.active) {
            enemy = Enemy{};
            enemy.id = world.nextEnemyId++;
            enemy.position = pos;
            enemy.active = true;
            return true;
        }
    }

    Enemy enemy;
    enemy.id = world.nextEnemyId++;
    enemy.position = pos;
    enemy.active = true;
    world.enemies.push_back(enemy);
    return true;
}

}  // namespace

GodApplyOutcome applyGodDecision(
    WorldState& world,
    const GodDecision& decision,
    EventSystem& events,
    MazeGenerator& gen,
    int gameTime) {
    GodApplyOutcome outcome;

    auto finishApplied = [&](GodApplyOutcome applied) {
        world.godFavor = addGodFavor(world.godFavor, clampFavorDelta(decision.favorDelta));
        return applied;
    };

    if (decision.action == GodActionType::None) {
        outcome.result = GodApplyResult::Applied;
        return finishApplied(outcome);
    }

    const int cost = expectedPowerCost(decision.action);
    if (world.godPower < cost) {
        outcome.result = GodApplyResult::RejectedInsufficientPower;
        return outcome;
    }

    std::mt19937 rng = decisionRng(world, gameTime);

    switch (decision.action) {
        case GodActionType::SendMessage: {
            world.godPower -= cost;
            std::ostringstream desc;
            desc << "THE GOD spoke: \"" << decision.message << "\"";
            events.record(EventType::GodMessageSent, gameTime, desc.str());
            syncRecentEvents(world, events);
            outcome.result = GodApplyResult::Applied;
            outcome.displayMessage = decision.message;
            return finishApplied(outcome);
        }
        case GodActionType::SpawnEnemy: {
            if (!isValidSpawnParameter(decision.parameter) || activeEnemyCount(world.enemies) >= MAX_ENEMIES) {
                outcome.result = GodApplyResult::RejectedInvalid;
                return outcome;
            }
            auto target = pickSpawnTarget(world, decision.parameter, rng);
            if (!target || !spawnEnemyAt(world, *target)) {
                outcome.result = GodApplyResult::RejectedInvalid;
                return outcome;
            }
            world.godPower -= cost;
            std::ostringstream desc;
            desc << "An enemy spawned at (" << target->x << ", " << target->y << ").";
            events.record(EventType::EnemySpawned, gameTime, desc.str());
            syncRecentEvents(world, events);
            outcome.result = GodApplyResult::Applied;
            return finishApplied(outcome);
        }
        case GodActionType::TeleportPlayer: {
            if (!isValidTeleportParameter(decision.parameter)) {
                outcome.result = GodApplyResult::RejectedInvalid;
                return outcome;
            }
            auto target = pickTeleportTarget(world, decision.parameter, rng);
            if (!target || !world.maze.isWalkable(*target)) {
                outcome.result = GodApplyResult::RejectedInvalid;
                return outcome;
            }
            world.player.position = *target;
            world.godPower -= cost;
            std::ostringstream desc;
            desc << "Player teleported to (" << target->x << ", " << target->y << ") via " << decision.parameter
                 << ".";
            events.record(EventType::PlayerTeleported, gameTime, desc.str());
            syncRecentEvents(world, events);
            outcome.result = GodApplyResult::Applied;
            return finishApplied(outcome);
        }
        case GodActionType::RegenerateMaze: {
            const uint32_t newSeed = world.mazeSeed * 1664525u + 1013904223u + 1u;
            rebuildMaze(world, gen, newSeed, true);
            world.profile.mazeRegenerationsExperienced += 1;
            world.currentLevel += 1;
            world.godPower -= cost;
            events.record(EventType::MazeRegenerated, gameTime, "The maze was regenerated by the God.");
            syncRecentEvents(world, events);
            outcome.result = GodApplyResult::Applied;
            return finishApplied(outcome);
        }
        case GodActionType::None:
            break;
    }

    outcome.result = GodApplyResult::RejectedInvalid;
    return outcome;
}
