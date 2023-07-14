#include <raylib.h>
#include <emscripten/emscripten.h>

#include "game.cpp"

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE);

  game_init();
  emscripten_set_main_loop(game_update_and_render, TARGET_FPS, 1);
  
  CloseWindow();
  return 0;
}