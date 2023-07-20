#include <raylib.h>
#include <emscripten/emscripten.h>

#include "game.cpp"

int main() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE);
  InitAudioDevice();

  init_game();
  emscripten_set_main_loop(do_game_loop, TARGET_FPS, 1);
  
  CloseWindow();
  return 0;
}