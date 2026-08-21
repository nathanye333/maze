#pragma once

#include "God.h"

#include <optional>

class MockGod : public God {
public:
    void beginEvaluate(
        const WorldState& world, std::string_view trigger, std::string_view playerMessage) override;
    bool isBusy() const override;
    bool tryTakeDecision(GodDecision& out) override;

private:
    GodDecision decide(const WorldState& world, std::string_view trigger, std::string_view playerMessage);

    std::optional<GodDecision> pending_;
    int lastMessageDamageBand_ = 0;
    int lastSpawnKillCount_ = -1;
    uint32_t lastRegenSeed_ = 0;
};
