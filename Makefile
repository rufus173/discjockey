CFLAGS=-g -Wall -Wextra `pkg-config --cflags sdl2 SDL2_mixer`
LFLAGS=`pkg-config --libs sdl2 SDL2_mixer` -lncursesw -fsanitize=address
discjockey : src/main.o src/queue.o src/visualiser.o
	$(CC) -o $@ $^ $(LFLAGS)
