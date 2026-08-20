#pragma once

#include <string>

enum class EventType {
    MazeStarted,
    PlayerMoved,
    EnemyKilled,
    PlayerDamaged,
    PlayerDied,
    PlayerReachedShrine,
    PlayerCommunicatedWithGod,
    PlayerReachedExit,
    MazeRegenerated,
    PlayerTeleported,
    EnemySpawned,
    GodMessageSent
};

struct GameEvent {
    EventType type{};
    int gameTime = 0;
    std::string description;
};

std::string eventTypeName(EventType type);
