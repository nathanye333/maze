#include "Game.h"

#include "Enemy.h"
#include "MockGod.h"
#include "Player.h"
#include "Shrine.h"

#include <raylib.h>

#include <algorithm>
#include <sstream>

namespace {
constexpr int kScreenWidth = 800;
constexpr int kScreenHeight = 600;
constexpr int kCellSize = 16;
constexpr float kGodMessageDuration = 4.0f;
constexpr int kCloseToExitDistance = 6;
constexpr int kCloseToExitResetDistance = 10;
}  // namespace

Game::Game() : rng_(1), god_(std::make_unique<MockGod>()) {
    initializeWorld(world_, generator_, events_, 1, 0);
}

void Game::run() {
    InitWindow(kScreenWidth, kScreenHeight, "Qwen Maze");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        update(GetFrameTime());
        draw();
    }

    CloseWindow();
}

void Game::requestGodEvaluation(int gameTime) {
    syncRecentEvents(world_, events_);
    const GodDecision decision = god_->evaluate(world_);
    if (decision.action == GodActionType::None) {
        return;
    }
    const GodApplyOutcome outcome = applyGodDecision(world_, decision, events_, generator_, gameTime);
    if (outcome.result == GodApplyResult::Applied && !outcome.displayMessage.empty()) {
        godMessage_ = outcome.displayMessage;
        godMessageTimer_ = kGodMessageDuration;
    }
    if (decision.action == GodActionType::RegenerateMaze && outcome.result == GodApplyResult::Applied) {
        closeToExitTriggered_ = false;
        events_.record(EventType::MazeStarted, gameTime, "The player continues in a new maze.");
        syncRecentEvents(world_, events_);
    }
}

bool Game::isCloseToExit() const {
    if (auto d = world_.maze.bfsDistance(world_.player.position, world_.maze.exitPosition())) {
        return *d <= kCloseToExitDistance;
    }
    return manhattan(world_.player.position, world_.maze.exitPosition()) <= kCloseToExitDistance;
}

void Game::onPlayerDeath(int gameTime) {
    events_.record(EventType::PlayerDied, gameTime, "The player died.");
    world_.profile.deaths += 1;
    syncRecentEvents(world_, events_);
    const uint32_t seedBefore = world_.mazeSeed;
    requestGodEvaluation(gameTime);
    if (world_.mazeSeed == seedBefore) {
        restartCurrentMaze(world_, generator_, events_, gameTime);
    } else {
        world_.player.health = PLAYER_MAX_HEALTH;
    }
    events_.record(EventType::MazeStarted, gameTime, "The maze restarted after death.");
    syncRecentEvents(world_, events_);
}

void Game::tryKillAdjacentEnemy(int gameTime) {
    for (Enemy& enemy : world_.enemies) {
        if (!enemy.active) {
            continue;
        }
        if (manhattan(enemy.position, world_.player.position) <= 1) {
            enemy.active = false;
            world_.profile.enemiesKilled += 1;
            std::ostringstream desc;
            desc << "Player killed an enemy at (" << enemy.position.x << ", " << enemy.position.y << ").";
            events_.record(EventType::EnemyKilled, gameTime, desc.str());
            syncRecentEvents(world_, events_);
            if (world_.profile.enemiesKilled - lastKillEvalCount_ >= 3) {
                lastKillEvalCount_ = world_.profile.enemiesKilled;
                requestGodEvaluation(gameTime);
            }
            return;
        }
    }
}

void Game::updateShrines(int gameTime) {
    nearShrine_ = findNearbyShrine(world_.shrines, world_.player.position) != nullptr;
    if (Shrine* shrine = findNearbyShrine(world_.shrines, world_.player.position)) {
        if (!shrine->reachedAnnounced) {
            shrine->reachedAnnounced = true;
            world_.profile.shrinesVisited += 1;
            std::ostringstream desc;
            desc << "Player reached shrine at (" << shrine->position.x << ", " << shrine->position.y << ").";
            events_.record(EventType::PlayerReachedShrine, gameTime, desc.str());
            syncRecentEvents(world_, events_);
            requestGodEvaluation(gameTime);
        }
    }
}

void Game::checkExit(int gameTime) {
    if (world_.player.position == world_.maze.exitPosition()) {
        escaped_ = true;
        events_.record(EventType::PlayerReachedExit, gameTime, "Player reached the exit.");
        syncRecentEvents(world_, events_);
    }
}

void Game::handleInput(float dt) {
    const int gameTime = static_cast<int>(elapsed_);
    world_.player.moveCooldown -= dt;
    if (world_.player.moveCooldown < 0.0f) {
        world_.player.moveCooldown = 0.0f;
    }

    int dx = 0;
    int dy = 0;
    if (IsKeyDown(KEY_W)) {
        dy = -1;
    } else if (IsKeyDown(KEY_S)) {
        dy = 1;
    } else if (IsKeyDown(KEY_A)) {
        dx = -1;
    } else if (IsKeyDown(KEY_D)) {
        dx = 1;
    }

    if ((dx != 0 || dy != 0) && world_.player.moveCooldown <= 0.0f) {
        if (tryMovePlayer(world_.player, world_.maze, dx, dy)) {
            world_.player.moveCooldown = PLAYER_MOVE_COOLDOWN;
            world_.profile.distanceTravelled += 1;
            if (world_.profile.distanceTravelled % 10 == 0) {
                std::ostringstream desc;
                desc << "Player moved to (" << world_.player.position.x << ", " << world_.player.position.y << ").";
                events_.record(EventType::PlayerMoved, gameTime, desc.str());
                syncRecentEvents(world_, events_);
            }
            checkExit(gameTime);
            updateShrines(gameTime);

            if (auto d = world_.maze.bfsDistance(world_.player.position, world_.maze.exitPosition())) {
                if (*d <= kCloseToExitDistance && !closeToExitTriggered_) {
                    closeToExitTriggered_ = true;
                    requestGodEvaluation(gameTime);
                } else if (*d >= kCloseToExitResetDistance) {
                    closeToExitTriggered_ = false;
                }
            }
        }
    }

    if (IsKeyPressed(KEY_E) && nearShrine_) {
        if (Shrine* shrine = findNearbyShrine(world_.shrines, world_.player.position)) {
            shrine->visited = true;
        }
        world_.profile.godInteractions += 1;
        events_.record(EventType::PlayerCommunicatedWithGod, gameTime, "Player spoke to the God at a shrine.");
        syncRecentEvents(world_, events_);
        requestGodEvaluation(gameTime);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        tryKillAdjacentEnemy(gameTime);
    }
}

void Game::stepEnemies(float dt) {
    const int gameTime = static_cast<int>(elapsed_);
    for (Enemy& enemy : world_.enemies) {
        if (!enemy.active) {
            continue;
        }
        enemy.moveCooldown -= dt;
        enemy.attackCooldown -= dt;

        if (enemy.position == world_.player.position && enemy.attackCooldown <= 0.0f) {
            world_.player.health -= ENEMY_DAMAGE;
            world_.profile.damageTaken += ENEMY_DAMAGE;
            enemy.attackCooldown = ENEMY_ATTACK_COOLDOWN;
            std::ostringstream desc;
            desc << "Player took " << ENEMY_DAMAGE << " damage at (" << world_.player.position.x << ", "
                 << world_.player.position.y << ").";
            events_.record(EventType::PlayerDamaged, gameTime, desc.str());
            syncRecentEvents(world_, events_);
            if (world_.player.health <= 0) {
                world_.player.health = 0;
                onPlayerDeath(gameTime);
                return;
            }
        }

        if (enemy.moveCooldown <= 0.0f) {
            enemy.position = chooseEnemyMove(enemy, world_.maze, world_.player.position, rng_);
            enemy.moveCooldown = ENEMY_MOVE_COOLDOWN;
        }
    }
}

void Game::update(float dt) {
    if (escaped_) {
        return;
    }

    elapsed_ += dt;
    const int gameTime = static_cast<int>(elapsed_);

    godPowerTimer_ += dt;
    if (godPowerTimer_ >= GOD_POWER_REGEN_INTERVAL) {
        godPowerTimer_ -= GOD_POWER_REGEN_INTERVAL;
        world_.godPower = addGodPower(world_.godPower, 1);
    }

    godEvalTimer_ += dt;
    if (godEvalTimer_ >= GOD_EVALUATION_INTERVAL) {
        godEvalTimer_ = 0.0f;
        requestGodEvaluation(gameTime);
    }

    if (godMessageTimer_ > 0.0f) {
        godMessageTimer_ -= dt;
        if (godMessageTimer_ <= 0.0f) {
            godMessage_.clear();
        }
    }

    handleInput(dt);
    if (!escaped_) {
        stepEnemies(dt);
        nearShrine_ = findNearbyShrine(world_.shrines, world_.player.position) != nullptr;
    }
}

void Game::draw() {
    BeginDrawing();
    ClearBackground(BLACK);

    Camera2D camera{};
    camera.target = {
        world_.player.position.x * static_cast<float>(kCellSize) + kCellSize / 2.0f,
        world_.player.position.y * static_cast<float>(kCellSize) + kCellSize / 2.0f};
    camera.offset = {kScreenWidth / 2.0f, kScreenHeight / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    BeginMode2D(camera);

    const int viewCellsX = kScreenWidth / kCellSize + 4;
    const int viewCellsY = kScreenHeight / kCellSize + 4;
    const int minX = std::max(0, world_.player.position.x - viewCellsX / 2);
    const int maxX = std::min(world_.maze.width() - 1, world_.player.position.x + viewCellsX / 2);
    const int minY = std::max(0, world_.player.position.y - viewCellsY / 2);
    const int maxY = std::min(world_.maze.height() - 1, world_.player.position.y + viewCellsY / 2);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const Rectangle cell{
                static_cast<float>(x * kCellSize),
                static_cast<float>(y * kCellSize),
                static_cast<float>(kCellSize),
                static_cast<float>(kCellSize)};
            if (world_.maze.cellAt(x, y) == CellType::Wall) {
                DrawRectangleRec(cell, Color{50, 50, 55, 255});
            } else {
                DrawRectangleRec(cell, Color{12, 12, 16, 255});
            }
        }
    }

    const auto drawCell = [](GridPosition p, Color color) {
        DrawRectangle(
            p.x * kCellSize + 1, p.y * kCellSize + 1, kCellSize - 2, kCellSize - 2, color);
    };

    drawCell(world_.maze.exitPosition(), Color{40, 180, 70, 255});
    for (const Shrine& shrine : world_.shrines) {
        drawCell(shrine.position, Color{150, 70, 200, 255});
    }
    for (const Enemy& enemy : world_.enemies) {
        if (enemy.active) {
            drawCell(enemy.position, Color{200, 50, 50, 255});
        }
    }
    drawCell(world_.player.position, Color{50, 110, 220, 255});

    EndMode2D();

    DrawText(TextFormat("HP: %d", world_.player.health), 16, 16, 24, RAYWHITE);

    if (nearShrine_ && !escaped_) {
        const char* prompt = "Press E to speak to the God";
        const int width = MeasureText(prompt, 20);
        DrawText(prompt, (kScreenWidth - width) / 2, kScreenHeight - 56, 20, Color{200, 160, 255, 255});
    }

    const char* hints = nearShrine_ ? "[WASD] Move    [E] Interact    [Space] Attack"
                                    : "[WASD] Move    [Space] Attack";
    const int hintWidth = MeasureText(hints, 16);
    DrawText(hints, (kScreenWidth - hintWidth) / 2, kScreenHeight - 28, 16, LIGHTGRAY);

    if (!godMessage_.empty() && godMessageTimer_ > 0.0f) {
        const int boxW = 540;
        const int boxH = 110;
        const int boxX = (kScreenWidth - boxW) / 2;
        const int boxY = 48;
        DrawRectangle(boxX, boxY, boxW, boxH, Fade(BLACK, 0.88f));
        DrawRectangleLines(boxX, boxY, boxW, boxH, LIGHTGRAY);
        DrawText("THE GOD", boxX + 18, boxY + 14, 22, Color{190, 150, 255, 255});
        DrawText(TextFormat("\"%s\"", godMessage_.c_str()), boxX + 18, boxY + 54, 20, RAYWHITE);
    }

    if (escaped_) {
        DrawRectangle(0, 0, kScreenWidth, kScreenHeight, Fade(BLACK, 0.55f));
        const char* text = "YOU ESCAPED";
        const int width = MeasureText(text, 48);
        DrawText(text, (kScreenWidth - width) / 2, kScreenHeight / 2 - 24, 48, GREEN);
    }

    EndDrawing();
}
