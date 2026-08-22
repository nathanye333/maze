#include "CombatAnimation.h"

#include <algorithm>

namespace {

float defaultDuration(CombatAnimKind kind) {
    switch (kind) {
        case CombatAnimKind::SwordSwing:
            return 0.15f;
        case CombatAnimKind::GunMuzzleFlash:
            return 0.08f;
        case CombatAnimKind::HitFlash:
            return 0.12f;
    }
    return 0.1f;
}

}  // namespace

void spawnCombatAnimation(
    CombatVisualState& visuals,
    CombatAnimKind kind,
    GridPosition origin,
    GridPosition facing) {
    CombatAnimation animation;
    animation.kind = kind;
    animation.duration = defaultDuration(kind);
    animation.origin = origin;
    animation.facing = facing;
    visuals.active.push_back(animation);
}

void updateCombatAnimations(CombatVisualState& visuals, float dt) {
    for (CombatAnimation& animation : visuals.active) {
        animation.elapsed += dt;
    }
    visuals.active.erase(
        std::remove_if(
            visuals.active.begin(),
            visuals.active.end(),
            [](const CombatAnimation& animation) { return animation.elapsed >= animation.duration; }),
        visuals.active.end());
}
