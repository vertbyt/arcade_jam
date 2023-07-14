#include <raylib.h>
#include "game.cpp"

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, TITLE);
  SetTargetFPS(TARGET_FPS);
  
  game_init();
  while (!WindowShouldClose()) {
    game_update_and_render();
  }

  CloseWindow(); 
  
  return 0;
}