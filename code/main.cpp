#include <raylib.h>
#include "game.cpp"

int main() {

  InitAudioDevice();

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE);
  SetTargetFPS(TARGET_FPS);
  SetExitKey(0);
 
  init_game();
  while (!WindowShouldClose()) {
    do_game_loop();
  }

  CloseWindow(); 
  
  return 0;
}