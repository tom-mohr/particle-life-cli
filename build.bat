@echo off
if not exist dist mkdir dist
gcc -o dist/particle-life particle-life.c pdcurses/*.o getopt/*.o

