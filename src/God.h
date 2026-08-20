#pragma once

#include "WorldState.h"

#include <string>

constexpr float GOD_EVALUATION_INTERVAL = 15.0f;
constexpr float GOD_POWER_REGEN_INTERVAL = 30.0f;
constexpr int MAX_GOD_POWER = 10;
constexpr int COST_SEND_MESSAGE = 0;
constexpr int COST_SPAWN_ENEMY = 1;
constexpr int COST_TELEPORT = 2;
constexpr int COST_REGENERATE = 4;

enum class GodActionType {
    None,
    RegenerateMaze,
    TeleportPlayer,
    SpawnEnemy,
    SendMessage
};

struct GodDecision {
    GodActionType action = GodActionType::None;
    std::string parameter;
    std::string message;
    int powerCost = 0;
};

class God {
public:
    virtual ~God() = default;
    virtual GodDecision evaluate(const WorldState& world) = 0;
};

enum class GodApplyResult {
    Applied,
    RejectedInvalid,
    RejectedInsufficientPower
};

struct GodApplyOutcome {
    GodApplyResult result = GodApplyResult::RejectedInvalid;
    std::string displayMessage;
};

class MazeGenerator;
class EventSystem;

int expectedPowerCost(GodActionType action);
int addGodPower(int current, int amount);
bool isValidTeleportParameter(const std::string& parameter);
bool isValidSpawnParameter(const std::string& parameter);
GodApplyOutcome applyGodDecision(
    WorldState& world,
    const GodDecision& decision,
    EventSystem& events,
    MazeGenerator& gen,
    int gameTime);
