#include <SDL.h>
#include <locale.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include "queue.h"
#include <SDL_mixer.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <ncurses.h>

//TODO: pending designs
/*
     ##                              
     ####                         
     ######                        
     ########..                      
     ######...                       
     ####...                       
     ##...                        
     ...                             

                                                  
     ##.  ##.                                     
     ##.  ##.                                     
     ##.  ##.                                     
     ##.  ##.                                     
     ##.  ##.                                     
     ##.  ##.                                     
     ...  ...                                     
*/

#define sdlerror(str) fprintf(stderr,"%s: %s\n",str,SDL_GetError())
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

void queue_window_update(WINDOW *queue_window,struct music_queue *queue);
wchar_t *str_to_wchar(char *str);

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
	//====== load music ======
	struct music_queue queue;
	memset(&queue,0,sizeof(struct music_queue));
	int count = queue_load(argv+optind,argc-optind,&queue);
	printf("%d files loaded\n",count);
	if (shuffle) queue_shuffle(&queue);
	printf("====== queue ======\n");
	for (int i = 0; i < queue.song_count; i++){
		printf("%s\n",queue.song_names[i]);
	}
	//====== init ncurses ======
	setlocale(LC_ALL,"");
	initscr();
	keypad(stdscr,1);
	noecho();
	refresh();
	//create windows
	/*
	+-----+-----+-----+
	|settings   |     |
	|(loop etc.)|  q  |
	+-----+-----+  u  +
	|song name  |  e  |
	|artist     |  u  |
	+-----+-----+  e  +
	|playing or |     |
	|paused     |     |
	+-----+-----+-----+
	*/
	//column 1 witdh	row 1 height
	int c1w = COLS/3;	int r1h = LINES/3;
	int c2w = COLS/3;	int r2h = LINES/3;
	int c3w = COLS-c1w-c2w;	int r3h = LINES-r1h-r2h;
	WINDOW *queue_window = newwin(LINES,c3w,0,c1w+c2w-1);
	//====== run the update loop ======
	for (int loop = 1;loop;){
		//wait for poll to come back
		struct pollfd stdin_poll = {
			.fd = STDIN_FILENO,
			.events = POLLIN,
		};
		//                              msecs
		int result = poll(&stdin_poll,1,100);
		if (result < 0){
			perror("poll");
			break;
		}
		if (stdin_poll.revents & POLLIN){
			//====== handle input ======
			int ch = getch();
			switch (ch){
				case KEY_DOWN:
				//bounds check
				if (queue.current_song_index >= queue.song_count-1) break;
				queue.current_song_index++;
				break;
				case KEY_UP:
				//bounds check
				if (queue.current_song_index <= 0) break;
				queue.current_song_index--;
				break;
				case 'q':
				loop = 0;
				break;
			}
		}
		//====== update windows ======
		queue_window_update(queue_window,&queue);
	}
	//====== cleanup ======
	delwin(queue_window);
	endwin();
	queue_free(&queue);
	Mix_CloseAudio();
	SDL_Quit();
	return 0;
}
void queue_window_update(WINDOW *window,struct music_queue *queue){
	int width,height;
	getmaxyx(window,height,width);
	werase(window);
	box_set(window,0,0);
	mvwprintw(window,1,1,"%dx%d",width,height);
	//so like when you scroll down and the highlight moves down untill it
	//gets to the middle, then the list scrolls and the highlight stays in the middle
	//but then when it gets to the end it goes to the bottom
	int highlight_top_offset = (queue->current_song_index-height-2) ? MIN(queue->current_song_index,(height-2)/2) : 0;
	//====== print the queue ======
	int y = 0;
	for (int i = queue->current_song_index-highlight_top_offset; i < queue->song_count; i++){
		if (y >= height-2) break;
		//wow isnt this function name so easy to understand
		//wide char string to multi byte string
		//bro c programmers will do anything but write out the name in full
		mvwaddnwstr(window,y+1,1,str_to_wchar(queue->song_names[i]),width-2);
		y++;
	}
	wrefresh(window);
}
wchar_t *str_to_wchar(char *str){
	static wchar_t buffer[4096]; //probs big enough
	size_t count = mbstowcs(buffer,str,4096);
	if (count == -1) wcscpy(buffer,L"Error");
	return buffer;
}
