#include <SDL.h>
#include <SDL_mixer.h>
#include <unistd.h>
#define sdlerror(str) fprintf(stderr,"%s: %s\n",str,SDL_GetError())
int main(int argc, char **argv){
	if (argc < 2){
		fprintf(stderr,"No file provided.\n");
		return 1;
	}
	//====== get audio ready ======
	if (!SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1")){
		sdlerror("SDL_SetHint");
		return 1;
	}
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0){
		sdlerror("SDL_Init");
		return 1;
	}
	if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 4096) != 0){
		sdlerror("Mix_OpenAudio");
		return 1;
	}
	//====== load and play some music ======
	Mix_Music *song = Mix_LoadMUS(argv[1]);
	if (song == NULL){
		sdlerror("Mix_LoadMUS");
		return 1;
	}
	if (Mix_PlayMusic(song,0) < 0){
		sdlerror("PlayMusic");
		return 1;
	}
	//wait for song to finish
	sleep(1);
	for (;Mix_PlayingMusic(););
	Mix_FreeMusic(song);
	//====== cleanup ======
	Mix_CloseAudio();
	SDL_Quit();
	return 0;
}
