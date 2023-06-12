if (-Not (Test-Path dist)) { New-Item -ItemType Directory -Force -Path dist }
if (-Not (Test-Path dist/windows)) { New-Item -ItemType Directory -Force -Path dist/windows }
$LIBS = "lib\pdcurses\*.o", "lib\getopt\*.o"
$OUT = "dist\windows\particle-life.exe"
gcc -o $OUT src\main.c -I lib\pdcurses -I libs\getopt $LIBS

