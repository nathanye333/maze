#include "TestSupport.h"

#include "EventSystem.h"

#include <string>

void runEventTests(int& passed, int& failed) {
    EventSystem events;
    for (int i = 0; i < 35; ++i) {
        events.record(EventType::PlayerMoved, i, "Player moved to (" + std::to_string(i) + ", 0).");
    }
    CHECK(static_cast<int>(events.recent().size()) == MAX_RECENT_EVENTS, "event history is bounded to 30");
    CHECK(events.recent().front().gameTime == 5, "oldest events are dropped first");
    CHECK(events.recent().back().gameTime == 34, "newest event is retained");
    CHECK(events.recent().back().description.find("Player moved") != std::string::npos,
          "event description is meaningful");

    events.clear();
    events.record(EventType::PlayerReachedShrine, 12, "Player reached shrine at (31, 42).");
    CHECK(events.recent().size() == 1, "single shrine event is stored");
    CHECK(events.recent().front().description == "Player reached shrine at (31, 42).",
          "shrine event keeps its description");
    CHECK(eventTypeName(EventType::PlayerReachedShrine) == "PLAYER_REACHED_SHRINE",
          "event type name matches spec style");
}
