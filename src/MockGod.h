#pragma once

#include "God.h"

class MockGod : public God {
public:
    GodDecision evaluate(const WorldState& world) override;

private:
    int lastGodInteractions_ = 0;
    int lastMessageDamageBand_ = 0;
    int lastSpawnKillCount_ = -1;
    int lastTeleportInteractions_ = 0;
    uint32_t lastRegenSeed_ = 0;
};
