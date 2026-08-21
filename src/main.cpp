#include "EnvFile.h"
#include "Game.h"

int main() {
    loadEnvFromDefaultLocations();
    Game game;
    game.run();
    return 0;
}
