#pragma once

#include "Event.h"

#include <string>
#include <vector>

constexpr int MAX_RECENT_EVENTS = 30;

class EventSystem {
public:
    void record(EventType type, int gameTime, std::string description);
    const std::vector<GameEvent>& recent() const { return events_; }
    void clear() { events_.clear(); }

private:
    std::vector<GameEvent> events_;
};
