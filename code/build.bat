@echo off


set FLAGS=-Zi -Od -nologo  -Fe"arcade_game.exe"
set ADD_INC=/I ../include
set LIBS=../lib/raylibdll.lib

cd ../build
cl %FLAGS% %ADD_INC% ../code/main.cpp -link %LIBS%
cd ../code