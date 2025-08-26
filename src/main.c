#include <SDL.h>
#include "queue.h"
#include <SDL_mixer.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#define sdlerror(str) fprintf(stderr,"%s: %s\n",str,SDL_GetError())
void print_help(char *name){
	printf("usage: %s [options] <file 0> ... <file n>\n",name);
	printf("songs are loaded in the order specified in the command line,\nand alphabeticaly from folders\n");
	printf("options:\n");
	printf("	-s, --shuffle  : shuffle the loaded songs\n");
	printf("	-h, --help     : display this help text\n");
}
int main(int argc, char **argv){
	srandom(time(NULL));
	int shuffle = 0;
	//====== process arguments ======
	int option_index = 0;
	struct option long_opts[] = {
		{"shuffle",no_argument,0,'s'},
		{"help",no_argument,0,'h'},
		{0,0,0,0},
	};
	for (;;){
		int result = getopt_long(argc,argv,"sh",long_opts,&option_index);
		if (result == -1) break;
		switch (result){
			case 's':
			shuffle = 1;
			break;
			case 'h':
			print_help(argv[0]);
			return 0;
		}
	}
	if (argc-optind < 1){
		fprintf(stderr,"No files provided.\n");
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
	struct music_queue queue;
	memset(&queue,0,sizeof(struct music_queue));
	int count = queue_load(argv+optind,argc-optind,&queue);
	printf("%d files loaded\n",count);
	if (shuffle) queue_shuffle(&queue);
	printf("====== queue ======\n");
	for (int i = 0; i < queue.song_count; i++){
		printf("%s\n",queue.song_names[i]);
	}
	//====== cleanup ======
	queue_free(&queue);
	Mix_CloseAudio();
	SDL_Quit();
	return 0;
}
