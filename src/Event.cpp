#include "Event.h"

std::string eventTypeName(EventType type) {
    switch (type) {
        case EventType::MazeStarted:
            return "MAZE_STARTED";
        case EventType::PlayerMoved:
            return "PLAYER_MOVED";
        case EventType::EnemyKilled:
            return "ENEMY_KILLED";
        case EventType::PlayerAttacked:
            return "PLAYER_ATTACKED";
        case EventType::WeaponPickedUp:
            return "WEAPON_PICKED_UP";
        case EventType::WeaponDropped:
            return "WEAPON_DROPPED";
        case EventType::PlayerDamaged:
            return "PLAYER_DAMAGED";
        case EventType::PlayerDied:
            return "PLAYER_DIED";
        case EventType::PlayerReachedShrine:
            return "PLAYER_REACHED_SHRINE";
        case EventType::PlayerCommunicatedWithGod:
            return "PLAYER_COMMUNICATED_WITH_GOD";
        case EventType::PlayerReachedExit:
            return "PLAYER_REACHED_EXIT";
        case EventType::MazeRegenerated:
            return "MAZE_REGENERATED";
        case EventType::PlayerTeleported:
            return "PLAYER_TELEPORTED";
        case EventType::EnemySpawned:
            return "ENEMY_SPAWNED";
        case EventType::GodMessageSent:
            return "GOD_MESSAGE_SENT";
    }
    return "UNKNOWN";
}
