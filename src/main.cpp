#include "Game.h"

int main() {
    Game game;
    game.setup();
    game.run();
    game.waitExit();
    return 0;
}