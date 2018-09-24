@echo off
gcc main.c -o ./main -L"./" -lcsfml-graphics-2 -lcsfml-window-2 -lcsfml-system-2 -lm
pause
