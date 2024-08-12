all:
	gcc -Isrc/Include -Lsrc/lib -o output main.c -lmingw32 -lSDL2main -lSDL2