#pragma once

#include "WorldState.h"

#include <string>
#include <string_view>

constexpr float GOD_EVAL_INTERVAL_MIN = 20.0f;
constexpr float GOD_EVAL_INTERVAL_MAX = 45.0f;
constexpr float GOD_AWAIT_TIMEOUT = 60.0f;
constexpr float GOD_AWAIT_MIN_FLASH = 0.4f;
constexpr float GOD_POWER_REGEN_INTERVAL = 30.0f;
constexpr int MAX_GOD_POWER = 10;
constexpr int MIN_GOD_FAVOR = -50;
constexpr int MAX_GOD_FAVOR = 50;
constexpr int MAX_FAVOR_DELTA = 5;
constexpr int PRAYER_MAX_CHARS = 120;
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
    int favorDelta = 0;
};

class God {
public:
    virtual ~God() = default;
    virtual void beginEvaluate(
        const WorldState& world, std::string_view trigger, std::string_view playerMessage) = 0;
    virtual bool isBusy() const = 0;
    virtual bool tryTakeDecision(GodDecision& out) = 0;
    virtual void cancel() {}
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
int addGodFavor(int current, int amount);
int clampFavorDelta(int delta);
bool isValidTeleportParameter(const std::string& parameter);
bool isValidSpawnParameter(const std::string& parameter);
bool parseGodDecisionJson(std::string_view text, GodDecision& out);
GodApplyOutcome applyGodDecision(
    WorldState& world,
    const GodDecision& decision,
    EventSystem& events,
    MazeGenerator& gen,
    int gameTime);
