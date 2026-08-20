#pragma once

#include "EventSystem.h"
#include "God.h"
#include "MazeGenerator.h"
#include "WorldState.h"

#include <memory>
#include <random>
#include <string>

class Game {
public:
    Game();
    void run();

private:
    void update(float dt);
    void draw();
    void handleInput(float dt);
    void stepEnemies(float dt);
    void updateShrines(int gameTime);
    void checkExit(int gameTime);
    void requestGodEvaluation(int gameTime);
    void onPlayerDeath(int gameTime);
    void tryKillAdjacentEnemy(int gameTime);
    bool isCloseToExit() const;

    WorldState world_;
    EventSystem events_;
    MazeGenerator generator_;
    std::unique_ptr<God> god_;
    std::mt19937 rng_;

    float elapsed_ = 0.0f;
    float godEvalTimer_ = 0.0f;
    float godPowerTimer_ = 0.0f;
    std::string godMessage_;
    float godMessageTimer_ = 0.0f;
    bool escaped_ = false;
    bool nearShrine_ = false;
    bool closeToExitTriggered_ = false;
    int lastKillEvalCount_ = 0;
};
