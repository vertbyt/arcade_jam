#include <raylib.h>
#include <emscripten/emscripten.h>

#include "game.cpp"

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE);

  init_game();
  emscripten_set_main_loop(do_game_loop, TARGET_FPS, 1);
  
  CloseWindow();
  return 0;
}