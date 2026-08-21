#pragma once

#include "EventSystem.h"
#include "God.h"
#include "MazeGenerator.h"
#include "WorldState.h"

#include <memory>
#include <optional>
#include <random>
#include <string>

enum class GameMode {
    Playing,
    ComposingPrayer,
    AwaitingGod
};

class Game {
public:
    Game();
    void run();

private:
    void update(float dt);
    void draw();
    void handlePlayingInput(float dt);
    void handleComposeInput();
    void stepEnemies(float dt);
    void updateShrines(int gameTime);
    void checkExit(int gameTime);
    void beginGodEvaluation(const std::string& trigger, const std::string& playerMessage, int gameTime);
    void applyCompletedDecision(const GodDecision& decision, int gameTime);
    void finishAwait(const GodDecision& decision, int gameTime, bool timedOut);
    void showGodMessage(const std::string& message);
    void onPlayerDeath(int gameTime);
    void tryKillAdjacentEnemy(int gameTime);
    void scheduleNextAmbientEval();
    void drainStaleGodDecision();

    WorldState world_;
    EventSystem events_;
    MazeGenerator generator_;
    std::unique_ptr<God> god_;
    std::mt19937 rng_;

    GameMode mode_ = GameMode::Playing;
    float elapsed_ = 0.0f;
    float godEvalRemaining_ = GOD_EVAL_INTERVAL_MIN;
    float godPowerTimer_ = 0.0f;
    float awaitTimer_ = 0.0f;
    float flashPhase_ = 0.0f;
    std::string godMessage_;
    float godMessageTimer_ = 0.0f;
    std::string prayerBuffer_;
    std::optional<GodDecision> heldDecision_;
    std::string awaitTrigger_;
    int awaitGameTime_ = 0;
    bool escaped_ = false;
    bool nearShrine_ = false;
    bool lineOfSightActive_ = true;
};
