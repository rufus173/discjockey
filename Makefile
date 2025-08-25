CFLAGS=-g -Wall -Wextra `pkg-config --cflags sdl2 SDL2_mixer`
LFLAGS=`pkg-config --libs sdl2 SDL2_mixer`
discjockey : src/main.o
	$(CC) -o $@ $^ $(LFLAGS)
