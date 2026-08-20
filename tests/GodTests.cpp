#include "TestSupport.h"

#include "EventSystem.h"
#include "God.h"
#include "MazeGenerator.h"
#include "WorldState.h"

namespace {

WorldState makeWorld(MazeGenerator& gen, EventSystem& events, uint32_t seed = 1) {
    WorldState world;
    initializeWorld(world, gen, events, seed, 0);
    return world;
}

}  // namespace

void runGodTests(int& passed, int& failed) {
    MazeGenerator gen;
    EventSystem events;

    CHECK(expectedPowerCost(GodActionType::SendMessage) == 0, "send message costs 0");
    CHECK(expectedPowerCost(GodActionType::SpawnEnemy) == 1, "spawn enemy costs 1");
    CHECK(expectedPowerCost(GodActionType::TeleportPlayer) == 2, "teleport costs 2");
    CHECK(expectedPowerCost(GodActionType::RegenerateMaze) == 4, "regenerate costs 4");
    CHECK(addGodPower(10, 1) == 10, "god power cannot exceed max");
    CHECK(addGodPower(9, 1) == 10, "god power regenerates up to max");

    WorldState world = makeWorld(gen, events);
    CHECK(world.godPower == 10, "world starts at full god power");
    CHECK(world.maze.isWalkable(world.player.position), "player starts on floor");

    GodDecision message;
    message.action = GodActionType::SendMessage;
    message.message = "You are being watched.";
    message.powerCost = 99;
    GodApplyOutcome outcome = applyGodDecision(world, message, events, gen, 1);
    CHECK(outcome.result == GodApplyResult::Applied, "send message is applied");
    CHECK(world.godPower == 10, "send message spends 0 power");
    CHECK(outcome.displayMessage == "You are being watched.", "send message returns display text");

    const int powerBeforeSpawn = world.godPower;
    const int enemiesBefore = activeEnemyCount(world.enemies);
    GodDecision spawn;
    spawn.action = GodActionType::SpawnEnemy;
    spawn.parameter = "random";
    outcome = applyGodDecision(world, spawn, events, gen, 2);
    CHECK(outcome.result == GodApplyResult::Applied, "spawn enemy is applied");
    CHECK(world.godPower == powerBeforeSpawn - 1, "spawn enemy spends 1 power");
    CHECK(activeEnemyCount(world.enemies) == enemiesBefore + 1, "spawn adds an active enemy");
    for (const Enemy& enemy : world.enemies) {
        if (enemy.active) {
            CHECK(world.maze.isWalkable(enemy.position), "spawned enemy is on a floor cell");
            CHECK(enemy.position != world.player.position, "spawned enemy is not on the player");
        }
    }

    const GridPosition beforeTeleport = world.player.position;
    const int powerBeforeTeleport = world.godPower;
    GodDecision teleport;
    teleport.action = GodActionType::TeleportPlayer;
    teleport.parameter = "dead_end";
    outcome = applyGodDecision(world, teleport, events, gen, 3);
    CHECK(outcome.result == GodApplyResult::Applied, "teleport is applied");
    CHECK(world.godPower == powerBeforeTeleport - 2, "teleport spends 2 power");
    CHECK(world.maze.isWalkable(world.player.position), "teleport never puts the player in a wall");
    CHECK(world.player.position != beforeTeleport || world.maze.walkableNeighborCount(beforeTeleport) == 1,
          "dead-end teleport moves the player or they were already in a dead end");

    for (const char* param : {"random_safe", "far_from_exit", "near_enemy"}) {
        teleport.parameter = param;
        outcome = applyGodDecision(world, teleport, events, gen, 4);
        if (outcome.result == GodApplyResult::Applied) {
            CHECK(world.maze.isWalkable(world.player.position), "teleport parameter keeps player on floor");
        } else {
            CHECK(outcome.result == GodApplyResult::RejectedInsufficientPower,
                  "teleport only fails for power once the maze is valid");
        }
    }

    world.godPower = 10;
    const int healthBefore = world.player.health = 77;
    const uint32_t seedBefore = world.mazeSeed;
    GodDecision regen;
    regen.action = GodActionType::RegenerateMaze;
    outcome = applyGodDecision(world, regen, events, gen, 5);
    CHECK(outcome.result == GodApplyResult::Applied, "regenerate maze is applied");
    CHECK(world.godPower == 6, "regenerate spends 4 power");
    CHECK(world.mazeSeed != seedBefore, "regenerate uses a new seed");
    CHECK(world.player.health == healthBefore, "regenerate does not kill or heal the player");
    CHECK(world.maze.isWalkable(world.player.position), "regenerate places the player on a floor cell");
    CHECK(world.maze.isWalkable(world.maze.exitPosition()), "regenerate places the exit on a floor cell");
    CHECK(gen.isSolvable(world.maze, world.player.position, world.maze.exitPosition()),
          "regenerate always creates a solvable maze");

    world.godPower = 0;
    spawn.parameter = "near_player";
    outcome = applyGodDecision(world, spawn, events, gen, 6);
    CHECK(outcome.result == GodApplyResult::RejectedInsufficientPower, "god cannot spend more than current power");
    CHECK(world.godPower == 0, "rejected action spends no power");

    world.godPower = 10;
    while (activeEnemyCount(world.enemies) < MAX_ENEMIES) {
        spawn.parameter = "far_from_player";
        outcome = applyGodDecision(world, spawn, events, gen, 7);
        if (outcome.result != GodApplyResult::Applied) {
            break;
        }
    }
    const int atCap = activeEnemyCount(world.enemies);
    CHECK(atCap == MAX_ENEMIES, "can spawn up to max enemies");
    spawn.parameter = "random";
    outcome = applyGodDecision(world, spawn, events, gen, 8);
    CHECK(outcome.result == GodApplyResult::RejectedInvalid, "spawn is rejected at max enemies");
    CHECK(activeEnemyCount(world.enemies) == atCap, "rejected spawn does not add an enemy");

    world.godPower = 10;
    teleport.parameter = "outside_map";
    outcome = applyGodDecision(world, teleport, events, gen, 9);
    CHECK(outcome.result == GodApplyResult::RejectedInvalid, "invalid teleport parameter is rejected");

    spawn.parameter = "on_player";
    outcome = applyGodDecision(world, spawn, events, gen, 10);
    CHECK(outcome.result == GodApplyResult::RejectedInvalid, "invalid spawn parameter is rejected");
}
