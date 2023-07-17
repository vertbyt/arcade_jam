#include <raylib.h>
#include "game.cpp"

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE);
  SetTargetFPS(TARGET_FPS);
  
  init_game();
  while (!WindowShouldClose()) {
    do_game_loop();
  }

  CloseWindow(); 
  
  return 0;
}