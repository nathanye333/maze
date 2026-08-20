#include "EventSystem.h"

void EventSystem::record(EventType type, int gameTime, std::string description) {
    events_.push_back(GameEvent{type, gameTime, std::move(description)});
    while (static_cast<int>(events_.size()) > MAX_RECENT_EVENTS) {
        events_.erase(events_.begin());
    }
}
