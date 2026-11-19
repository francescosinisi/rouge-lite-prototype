#include <raylib.h>
#include "game.h"

int main() {
    InitWindow(1280, 720, "Window");
    SetTargetFPS(60);

    GameState state = {};

    game_init(&state);

    while(!WindowShouldClose()) {
        game_update(&state);
    }

    game_shutdown(&state);
    CloseWindow();

    return 0;
}
