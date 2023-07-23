@echo off

set FLAGS=-std=c++14 -Os -Wall  -Wno-missing-braces
set FLAGS=%FLAGS% ../lib/libraylib.a -I. -I../include/ -L. -L../lib/libraylib.a 
set FLAGS=%FLAGS% -s TOTAL_MEMORY=67108864 -s USE_GLFW=3 --shell-file minshell.html
set FLAGS=%FLAGS% --preload-file ../run_tree/imgs/ --preload-file ../run_tree/fonts/
set FLAGS=%FLAGS% --preload-file ../run_tree/audio/ 
set FLAGS=%FLAGS% -DPLATFORM_WEB


cd ../build_web
call "emcc" -o game.html ../code/main_web.cpp %FLAGS%
cd ../code