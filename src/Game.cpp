#include "Game.h"

#include "Combat.h"
#include "Enemy.h"
#include "MockGod.h"
#include "OllamaGod.h"
#include "Player.h"
#include "Shrine.h"
#include "Vision.h"
#include "Weapon.h"
#include "WeaponPickup.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kCellSize = 16;
constexpr float kGodMessageDuration = 4.0f;

std::unique_ptr<God> makeGod() {
    const char* godMode = std::getenv("QWEN_MAZE_GOD");
    const char* modelEnv = std::getenv("OLLAMA_MODEL");
    const bool useOllama =
        (godMode != nullptr && std::strcmp(godMode, "ollama") == 0) || (modelEnv != nullptr && modelEnv[0] != '\0');
    if (!useOllama) {
        return std::make_unique<MockGod>();
    }

    std::string host = "127.0.0.1";
    int port = 11434;
    if (const char* hostEnv = std::getenv("OLLAMA_HOST")) {
        std::string value = hostEnv;
        if (value.rfind("http://", 0) == 0) {
            value = value.substr(7);
        } else if (value.rfind("https://", 0) == 0) {
            value = value.substr(8);
        }
        const size_t colon = value.find(':');
        if (colon == std::string::npos) {
            host = value;
        } else {
            host = value.substr(0, colon);
            port = std::atoi(value.substr(colon + 1).c_str());
            if (port <= 0) {
                port = 11434;
            }
        }
    }

    const std::string model = (modelEnv != nullptr && modelEnv[0] != '\0') ? modelEnv : "qwen3.6";
    return std::make_unique<OllamaGod>(host, port, model);
}

bool envFlagEnabled(const char* name, bool defaultValue) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return defaultValue;
    }
    std::string flag = value;
    for (char& ch : flag) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    if (flag == "1" || flag == "true" || flag == "yes" || flag == "on") {
        return true;
    }
    if (flag == "0" || flag == "false" || flag == "no" || flag == "off") {
        return false;
    }
    return defaultValue;
}

Color favorTint(int favor) {
    const float t = (static_cast<float>(favor - MIN_GOD_FAVOR) / static_cast<float>(MAX_GOD_FAVOR - MIN_GOD_FAVOR));
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    if (clamped < 0.5f) {
        const float u = clamped * 2.0f;
        return Color{
            static_cast<unsigned char>(180 + static_cast<int>((80 - 180) * u)),
            static_cast<unsigned char>(20 + static_cast<int>((70 - 20) * u)),
            static_cast<unsigned char>(30 + static_cast<int>((140 - 30) * u)),
            255};
    }
    const float u = (clamped - 0.5f) * 2.0f;
    return Color{
        static_cast<unsigned char>(80 + static_cast<int>((220 - 80) * u)),
        static_cast<unsigned char>(70 + static_cast<int>((180 - 70) * u)),
        static_cast<unsigned char>(140 + static_cast<int>((70 - 140) * u)),
        255};
}

Camera2D makeCamera(int screenW, int screenH, GridPosition playerPos) {
    Camera2D camera{};
    camera.target = {
        playerPos.x * static_cast<float>(kCellSize) + kCellSize / 2.0f,
        playerPos.y * static_cast<float>(kCellSize) + kCellSize / 2.0f};
    camera.offset = {screenW / 2.0f, screenH / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    return camera;
}

Color weaponPickupColor(const std::string& weaponId) {
    if (weaponId == "gun") {
        return Color{220, 140, 40, 255};
    }
    return Color{180, 180, 190, 255};
}

void drawCombatAnimations(const CombatVisualState& visuals) {
    for (const CombatAnimation& animation : visuals.active) {
        const float t = animation.elapsed / animation.duration;
        const unsigned char alpha = static_cast<unsigned char>((1.0f - t) * 255.0f);
        const float cx = animation.origin.x * static_cast<float>(kCellSize) + kCellSize / 2.0f;
        const float cy = animation.origin.y * static_cast<float>(kCellSize) + kCellSize / 2.0f;

        switch (animation.kind) {
            case CombatAnimKind::SwordSwing: {
                const float tx = cx + animation.facing.x * kCellSize * 0.6f;
                const float ty = cy + animation.facing.y * kCellSize * 0.6f;
                DrawLineEx({cx, cy}, {tx, ty}, 3.0f, Color{220, 220, 240, alpha});
                break;
            }
            case CombatAnimKind::GunMuzzleFlash: {
                const float fx = cx + animation.facing.x * kCellSize * 0.55f;
                const float fy = cy + animation.facing.y * kCellSize * 0.55f;
                DrawRectangle(
                    static_cast<int>(fx - 3),
                    static_cast<int>(fy - 3),
                    6,
                    6,
                    Color{255, 230, 80, alpha});
                break;
            }
            case CombatAnimKind::HitFlash: {
                DrawRectangle(
                    animation.origin.x * kCellSize + 1,
                    animation.origin.y * kCellSize + 1,
                    kCellSize - 2,
                    kCellSize - 2,
                    Color{255, 255, 255, static_cast<unsigned char>(alpha / 2)});
                break;
            }
        }
    }
}

void drawAimIndicator(GridPosition player, GridPosition facing) {
    const float cx = player.x * static_cast<float>(kCellSize) + kCellSize / 2.0f;
    const float cy = player.y * static_cast<float>(kCellSize) + kCellSize / 2.0f;
    const float tx = cx + facing.x * kCellSize * 0.75f;
    const float ty = cy + facing.y * kCellSize * 0.75f;
    DrawLineEx({cx, cy}, {tx, ty}, 2.0f, Color{120, 170, 255, 180});
    DrawCircleV({tx, ty}, 2.5f, Color{120, 170, 255, 220});
}

void drawCrosshair(Vector2 screenPos) {
    DrawLineV({screenPos.x - 8.0f, screenPos.y}, {screenPos.x + 8.0f, screenPos.y}, Color{255, 255, 255, 210});
    DrawLineV({screenPos.x, screenPos.y - 8.0f}, {screenPos.x, screenPos.y + 8.0f}, Color{255, 255, 255, 210});
    DrawCircleLinesV(screenPos, 6.0f, Color{255, 255, 255, 180});
}

}  // namespace

Game::Game() : god_(makeGod()), rng_(std::random_device{}()) {
    lineOfSightActive_ = envFlagEnabled("QWEN_MAZE_LOS", true);
    initializeWorld(world_, generator_, events_, 1, 0);
    scheduleNextAmbientEval();
}

void Game::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(kScreenWidth, kScreenHeight, "Qwen Maze");
    SetWindowMinSize(640, 360);
    SetTargetFPS(60);
    HideCursor();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }
        update(GetFrameTime());
        draw();
    }

    ShowCursor();
    CloseWindow();
}

void Game::scheduleNextAmbientEval() {
    std::uniform_real_distribution<float> dist(GOD_EVAL_INTERVAL_MIN, GOD_EVAL_INTERVAL_MAX);
    godEvalRemaining_ = dist(rng_);
}

void Game::drainStaleGodDecision() {
    GodDecision discarded;
    if (god_->tryTakeDecision(discarded)) {
        // Drop late results after timeout/cancel.
    }
}

void Game::beginGodEvaluation(const std::string& trigger, const std::string& playerMessage, int gameTime) {
    if (mode_ == GameMode::AwaitingGod || god_->isBusy()) {
        return;
    }
    syncRecentEvents(world_, events_);
    heldDecision_.reset();
    awaitTimer_ = 0.0f;
    flashPhase_ = 0.0f;
    awaitGameTime_ = gameTime;
    awaitTrigger_ = trigger;
    god_->beginEvaluate(world_, trigger, playerMessage);
    mode_ = GameMode::AwaitingGod;
}

void Game::showGodMessage(const std::string& message) {
    if (message.empty()) {
        return;
    }
    godMessage_ = message;
    godMessageTimer_ = kGodMessageDuration;
}

void Game::applyCompletedDecision(const GodDecision& decision, int gameTime) {
    if (decision.action == GodActionType::None && decision.favorDelta == 0) {
        return;
    }
    const GodApplyOutcome outcome = applyGodDecision(world_, decision, events_, generator_, gameTime);
    if (outcome.result == GodApplyResult::Applied && !outcome.displayMessage.empty()) {
        showGodMessage(outcome.displayMessage);
    }
    if (decision.action == GodActionType::RegenerateMaze && outcome.result == GodApplyResult::Applied) {
        events_.record(EventType::MazeStarted, gameTime, "The player continues in a new maze.");
        syncRecentEvents(world_, events_);
    }
}

void Game::finishAwait(const GodDecision& decision, int gameTime, bool timedOut) {
    if (timedOut) {
        showGodMessage("The heavens are silent.");
    } else if (decision.action == GodActionType::None && awaitTrigger_ == "shrine") {
        showGodMessage("The heavens are silent.");
    } else {
        applyCompletedDecision(decision, gameTime);
    }
    heldDecision_.reset();
    awaitTrigger_.clear();
    mode_ = GameMode::Playing;
    scheduleNextAmbientEval();
}

void Game::onPlayerDeath(int gameTime) {
    events_.record(EventType::PlayerDied, gameTime, "The player died.");
    world_.profile.deaths += 1;
    world_.godFavor = addGodFavor(world_.godFavor, -1);
    syncRecentEvents(world_, events_);
    restartCurrentMaze(world_, generator_, events_, gameTime);
    events_.record(EventType::MazeStarted, gameTime, "The maze restarted after death.");
    syncRecentEvents(world_, events_);
}

void Game::updateAimFromMouse() {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    const Camera2D camera = makeCamera(screenW, screenH, world_.player.position);
    const Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    world_.inventory.facing =
        computeAimFacing(world_.player.position, mouseWorld.x, mouseWorld.y, kCellSize);
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

void Game::handleComposeInput() {
    if (!nearShrine_) {
        prayerBuffer_.clear();
        mode_ = GameMode::Playing;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        prayerBuffer_.clear();
        mode_ = GameMode::Playing;
        return;
    }

    int codepoint = GetCharPressed();
    while (codepoint > 0) {
        if (codepoint >= 32 && codepoint < 127 && static_cast<int>(prayerBuffer_.size()) < PRAYER_MAX_CHARS) {
            prayerBuffer_.push_back(static_cast<char>(codepoint));
        }
        codepoint = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !prayerBuffer_.empty()) {
        prayerBuffer_.pop_back();
    }

    if (IsKeyPressed(KEY_ENTER) && !prayerBuffer_.empty()) {
        const int gameTime = static_cast<int>(elapsed_);
        if (Shrine* shrine = findNearbyShrine(world_.shrines, world_.player.position)) {
            shrine->visited = true;
        }
        world_.profile.godInteractions += 1;
        world_.godFavor = addGodFavor(world_.godFavor, 1);
        std::ostringstream desc;
        desc << "Player spoke to the God at a shrine: \"" << prayerBuffer_ << "\".";
        events_.record(EventType::PlayerCommunicatedWithGod, gameTime, desc.str());
        syncRecentEvents(world_, events_);
        const std::string message = prayerBuffer_;
        prayerBuffer_.clear();
        beginGodEvaluation("shrine", message, gameTime);
    }
}

void Game::handlePlayingInput(float dt) {
    const int gameTime = static_cast<int>(elapsed_);
    updateAimFromMouse();

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
            tryCollectWeaponPickups(world_, events_, gameTime);
        }
    }

    if (IsKeyPressed(KEY_E)) {
        if (nearShrine_) {
            prayerBuffer_.clear();
            mode_ = GameMode::ComposingPrayer;
        } else if (!world_.inventory.weaponIds.empty()) {
            cycleEquippedWeapon(world_.inventory);
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        tryPlayerAttack(world_, events_, gameTime, world_.combatVisuals);
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

    if (godMessageTimer_ > 0.0f) {
        godMessageTimer_ -= dt;
        if (godMessageTimer_ <= 0.0f) {
            godMessage_.clear();
        }
    }

    if (mode_ == GameMode::AwaitingGod) {
        awaitTimer_ += dt;
        flashPhase_ += dt;

        GodDecision arrived;
        if (!heldDecision_ && god_->tryTakeDecision(arrived)) {
            heldDecision_ = arrived;
        }

        const bool ready = heldDecision_.has_value() && awaitTimer_ >= GOD_AWAIT_MIN_FLASH;
        const bool timedOut = awaitTimer_ >= GOD_AWAIT_TIMEOUT;
        if (ready) {
            finishAwait(*heldDecision_, awaitGameTime_, false);
        } else if (timedOut) {
            god_->cancel();
            drainStaleGodDecision();
            finishAwait(GodDecision{}, awaitGameTime_, true);
        }
        nearShrine_ = findNearbyShrine(world_.shrines, world_.player.position) != nullptr;
        return;
    }

    drainStaleGodDecision();

    if (mode_ == GameMode::ComposingPrayer) {
        handleComposeInput();
        nearShrine_ = findNearbyShrine(world_.shrines, world_.player.position) != nullptr;
        return;
    }

    godEvalRemaining_ -= dt;
    if (godEvalRemaining_ <= 0.0f) {
        beginGodEvaluation("ambient", "", gameTime);
        if (mode_ == GameMode::AwaitingGod) {
            nearShrine_ = findNearbyShrine(world_.shrines, world_.player.position) != nullptr;
            return;
        }
        scheduleNextAmbientEval();
    }

    handlePlayingInput(dt);
    if (!escaped_) {
        updateCombat(world_, events_, gameTime, dt, world_.combatVisuals);
        stepEnemies(dt);
        nearShrine_ = findNearbyShrine(world_.shrines, world_.player.position) != nullptr;
    }
}

void Game::draw() {
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    BeginDrawing();
    ClearBackground(BLACK);

    const Camera2D camera = makeCamera(screenW, screenH, world_.player.position);

    const std::vector<char> visibility =
        lineOfSightActive_ ? computeVisibility(world_.maze, world_.player.position)
                           : std::vector<char>{};
    const auto canSee = [&](GridPosition p) {
        return !lineOfSightActive_ || isVisible(visibility, world_.maze, p);
    };

    BeginMode2D(camera);

    const int viewCellsX = screenW / kCellSize + 4;
    const int viewCellsY = screenH / kCellSize + 4;
    const int minX = std::max(0, world_.player.position.x - viewCellsX / 2);
    const int maxX = std::min(world_.maze.width() - 1, world_.player.position.x + viewCellsX / 2);
    const int minY = std::max(0, world_.player.position.y - viewCellsY / 2);
    const int maxY = std::min(world_.maze.height() - 1, world_.player.position.y + viewCellsY / 2);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (lineOfSightActive_ && !isVisible(visibility, world_.maze, x, y)) {
                continue;
            }
            const Rectangle cell{
                static_cast<float>(x * kCellSize),
                static_cast<float>(y * kCellSize),
                static_cast<float>(kCellSize),
                static_cast<float>(kCellSize)};
            if (world_.maze.cellAt(x, y) == CellType::Wall) {
                DrawRectangleRec(cell, Color{50, 50, 55, 255});
            } else {
                DrawRectangleRec(cell, Color{18, 18, 24, 255});
            }
        }
    }

    const auto drawCell = [](GridPosition p, Color color) {
        DrawRectangle(p.x * kCellSize + 1, p.y * kCellSize + 1, kCellSize - 2, kCellSize - 2, color);
    };

    if (canSee(world_.maze.exitPosition())) {
        drawCell(world_.maze.exitPosition(), Color{40, 180, 70, 255});
    }
    for (const Shrine& shrine : world_.shrines) {
        if (canSee(shrine.position)) {
            drawCell(shrine.position, Color{150, 70, 200, 255});
        }
    }
    for (const WeaponPickup& pickup : world_.weaponPickups) {
        if (!pickup.collected && canSee(pickup.position)) {
            drawCell(pickup.position, weaponPickupColor(pickup.weaponId));
        }
    }
    for (const Enemy& enemy : world_.enemies) {
        if (enemy.active && canSee(enemy.position)) {
            const float hpRatio = std::clamp(
                static_cast<float>(enemy.health) / static_cast<float>(ENEMY_MAX_HEALTH), 0.0f, 1.0f);
            const unsigned char red = static_cast<unsigned char>(80 + hpRatio * 120);
            drawCell(enemy.position, Color{red, 50, 50, 255});
        }
    }
    for (const Projectile& projectile : world_.projectiles) {
        if (projectile.active && canSee(projectile.cell)) {
            DrawCircle(
                projectile.cell.x * kCellSize + kCellSize / 2,
                projectile.cell.y * kCellSize + kCellSize / 2,
                3.0f,
                Color{255, 240, 120, 255});
        }
    }
    drawCell(world_.player.position, Color{50, 110, 220, 255});

    if (mode_ == GameMode::Playing && equippedWeapon(world_.inventory) != nullptr) {
        drawAimIndicator(world_.player.position, world_.inventory.facing);
    }
    drawCombatAnimations(world_.combatVisuals);

    EndMode2D();

    DrawText(TextFormat("HP: %d", world_.player.health), 16, 16, 24, RAYWHITE);
    if (const WeaponDef* weapon = equippedWeapon(world_.inventory)) {
        DrawText(
            TextFormat("Weapon: %s", weapon->name.c_str()),
            16,
            44,
            18,
            RAYWHITE);
        if (world_.inventory.attackCooldown > 0.0f) {
            DrawText(
                TextFormat("Cooldown: %.1fs", world_.inventory.attackCooldown),
                16,
                66,
                16,
                LIGHTGRAY);
        }
    } else {
        DrawText("Weapon: Unarmed — find sword or gun", 16, 44, 18, LIGHTGRAY);
    }
    DrawText(TextFormat("Favor: %d", world_.godFavor), 16, 90, 18, favorTint(world_.godFavor));
    DrawText(lineOfSightActive_ ? "LOS: ON" : "LOS: OFF", 16, 114, 16, LIGHTGRAY);

    if (mode_ == GameMode::AwaitingGod) {
        const float favorNorm =
            static_cast<float>(world_.godFavor - MIN_GOD_FAVOR) / static_cast<float>(MAX_GOD_FAVOR - MIN_GOD_FAVOR);
        const float speed = 4.0f + (1.0f - std::clamp(favorNorm, 0.0f, 1.0f)) * 4.0f;
        const float pulse = 0.5f + 0.5f * std::sin(flashPhase_ * speed);
        const float alpha = 0.12f + 0.22f * pulse;
        Color tint = favorTint(world_.godFavor);
        tint.a = static_cast<unsigned char>(alpha * 255.0f);
        DrawRectangle(0, 0, screenW, screenH, tint);
        const char* waiting = "The God considers...";
        const int width = MeasureText(waiting, 28);
        DrawText(waiting, (screenW - width) / 2, 24, 28, RAYWHITE);
    }

    if (mode_ == GameMode::ComposingPrayer) {
        const int boxW = std::min(620, screenW - 40);
        const int boxH = 120;
        const int boxX = (screenW - boxW) / 2;
        const int boxY = screenH - boxH - 40;
        DrawRectangle(boxX, boxY, boxW, boxH, Fade(BLACK, 0.9f));
        DrawRectangleLines(boxX, boxY, boxW, boxH, Color{200, 160, 255, 255});
        DrawText("Speak, mortal:", boxX + 16, boxY + 14, 20, Color{200, 160, 255, 255});
        std::string shown = prayerBuffer_;
        shown.push_back('_');
        DrawText(shown.c_str(), boxX + 16, boxY + 52, 22, RAYWHITE);
        DrawText("[Enter] Send   [Esc] Cancel", boxX + 16, boxY + 88, 16, LIGHTGRAY);
    } else if (nearShrine_ && !escaped_ && mode_ == GameMode::Playing) {
        const char* prompt = "Press E to speak to the God";
        const int width = MeasureText(prompt, 20);
        DrawText(prompt, (screenW - width) / 2, screenH - 56, 20, Color{200, 160, 255, 255});
    }

    if (mode_ == GameMode::Playing) {
        drawCrosshair(GetMousePosition());
        const char* hints = nearShrine_ ? "[WASD] Move  [LMB] Attack  [E] Interact  [F11] Fullscreen"
                                        : "[WASD] Move  [LMB] Attack  [E] Equip  [F11] Fullscreen";
        const int hintWidth = MeasureText(hints, 16);
        DrawText(hints, (screenW - hintWidth) / 2, screenH - 28, 16, LIGHTGRAY);
    }

    if (!godMessage_.empty() && godMessageTimer_ > 0.0f) {
        const int boxW = std::min(540, screenW - 40);
        const int boxH = 110;
        const int boxX = (screenW - boxW) / 2;
        const int boxY = 48;
        DrawRectangle(boxX, boxY, boxW, boxH, Fade(BLACK, 0.88f));
        DrawRectangleLines(boxX, boxY, boxW, boxH, LIGHTGRAY);
        DrawText("THE GOD", boxX + 18, boxY + 14, 22, Color{190, 150, 255, 255});
        DrawText(TextFormat("\"%s\"", godMessage_.c_str()), boxX + 18, boxY + 54, 20, RAYWHITE);
    }

    if (escaped_) {
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.55f));
        const char* text = "YOU ESCAPED";
        const int width = MeasureText(text, 48);
        DrawText(text, (screenW - width) / 2, screenH / 2 - 24, 48, GREEN);
    }

    EndDrawing();
}
