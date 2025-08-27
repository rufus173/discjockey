#include <SDL.h>
#include <sys/ioctl.h>
#include <errno.h>
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

volatile sig_atomic_t sigwinch_occured = 0;

#define sdlerror(str) fprintf(stderr,"%s: %s\n",str,SDL_GetError())
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

enum colour_pairs {
	PAIR_DEFAULT = 1,
	PAIR_BLACK_WHITE,
	PAIR_DEFAULT_REVERSE,
};

void queue_window_update(WINDOW *queue_window,struct music_queue *queue);
void playback_status_window_update(WINDOW *playback_status_window,struct music_queue *queue);
void sigwinch_handler(int sig);
wchar_t *str_to_wchar(const char *str);

void print_help(char *name){
	printf("usage: %s [options] <file 0> ... <file n>\n",name);
	printf("songs are loaded in the order specified in the command line,\nand alphabeticaly from folders\n");
	printf("options:\n");
	printf("	-s, --shuffle          : shuffle the loaded songs\n");
	printf("	-h, --help             : display this help text\n");
	printf("	-d, --disable-autoplay : disable autoplay\n");
	printf("controls:\n");
	printf("	q : quit");
	printf("	up arrow : move cursor up");
	printf("	down arrow : move cursor down");
	printf("	enter : play selected song");
	printf("	space : pause/play");
	printf("	b : jump to current song");
}
int main(int argc, char **argv){
	srandom(time(NULL));
	int shuffle = 0;
	int autoplay = 1;
	//====== process arguments ======
	int option_index = 0;
	struct option long_opts[] = {
		{"shuffle",no_argument,0,'s'},
		{"help",no_argument,0,'h'},
		{"disable-autoplay",no_argument,0,'d'},
		{0,0,0,0},
	};
	for (;;){
		int result = getopt_long(argc,argv,"shd",long_opts,&option_index);
		if (result == -1) break;
		switch (result){
			case 's':
			shuffle = 1;
			break;
			case 'h':
			print_help(argv[0]);
			return 0;
			case 'd':
			autoplay = 0;
			break;
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
		printf("%s\n",queue.songs[i].name);
	}
	//====== init ncurses ======
	setlocale(LC_ALL,"");
	initscr();
	signal(SIGWINCH,sigwinch_handler);
	use_default_colors(); //transparent background
	start_color();
	//init_pair(PAIR_DEFAULT,-1,-1);
	init_pair(PAIR_DEFAULT,-1,-1);
	init_pair(PAIR_BLACK_WHITE,COLOR_BLACK,COLOR_WHITE);
	keypad(stdscr,1);
	noecho();
	refresh();
	//create windows
	/*
	+-----+-----+-----+
	|cover art  |     |
	|           |  q  |
	+           +  u  +
	|           |  e  |
	|           |  u  |
	+-----+-----+  e  +
	|playing or |     |
	|paused     |     |
	+-----+-----+-----+
	*/
	//rows and columns start from bottom left
	//column 1 witdh	row 1 height
	int c1w = COLS/3;	int r1h = LINES/3;
	int c2w = COLS/3;	int r2h = LINES/3;
	int c3w = COLS-c1w-c2w;	int r3h = LINES-r1h-r2h;
	WINDOW *queue_window = newwin(LINES,c3w,0,c1w+c2w-1);
	WINDOW *playback_status_window = newwin(r3h,c1w+c2w-1,r1h+r2h,0);
	//====== start the first song ======
	if (autoplay) queue_play(&queue);
	//====== run the update loop ======
	for (int loop = 1;loop;){
		//wait for poll to come back
		struct pollfd stdin_poll = {
			.fd = STDIN_FILENO,
			.events = POLLIN,
		};
		//                              msecs
		int result = poll(&stdin_poll,1,100);
		if (result < 0 && errno != EINTR){
			perror("poll");
			break;
		}
		if (stdin_poll.revents & POLLIN){
			//====== handle input ======
			int ch = getch();
			switch (ch){
				//====== scrolling up and down ======
				case KEY_DOWN:
				//bounds check
				if (queue.selected_song_index >= queue.song_count-1) break;
				queue.selected_song_index++;
				break;
				case KEY_UP:
				//bounds check
				if (queue.selected_song_index <= 0) break;
				queue.selected_song_index--;
				break;
				//====== quit ======
				case 'q':
				loop = 0;
				break;
				//====== back to current song ======
				case 'b':
				queue.selected_song_index = queue.current_song_index;
				break;
				//====== start song ======
				case '\n':
				queue_play(&queue);
				break;
				//====== pausing/resuming ======
				case ' ':
				queue_pause_resume(&queue);
				break;
			}
		}
		//====== autoplay ======
		if (!Mix_PlayingMusic()) queue.playback_status = PLAYBACK_STOPPED;
		if (autoplay && !Mix_PlayingMusic()) queue_next(&queue);
		//====== handle sigwinch ======
		if (sigwinch_occured){
			sigwinch_occured = 0;
			//resize the screen
			struct winsize size;
			int result = ioctl(STDOUT_FILENO,TIOCGWINSZ, &size);
			if (result == 0) resizeterm(size.ws_row,size.ws_col);
			//resize windows
			int lines,cols;
			getmaxyx(stdscr,lines,cols);
			int c1w = cols/3;	int r1h = lines/3;
			int c2w = cols/3;	int r2h = lines/3;
			int c3w = cols-c1w-c2w;	int r3h = lines-r1h-r2h;
			//delete old windows
			delwin(queue_window);
			delwin(playback_status_window);
			clear();
			refresh();
			//make new windows
			queue_window = newwin(LINES,c3w,0,c1w+c2w-1);
			playback_status_window = newwin(r3h,c1w+c2w-1,r1h+r2h,0);
		}
		//====== update windows ======
		playback_status_window_update(playback_status_window,&queue);
		//this goes last as it moves the cursor to its final position
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
	//im not realy sure how this one liner works but i just made it from experimentation
	int highlight_top_offset = (queue->song_count-queue->selected_song_index-(height%2) > (height-2)/2) ? MIN(queue->selected_song_index,((height-2)/2)) : ((height-2))-(queue->song_count-queue->selected_song_index);
	//====== print the queue ======
	int y = 0;
	int cursor_y = 0;
	for (int i = MAX(0,queue->selected_song_index-highlight_top_offset); i < queue->song_count; i++){
		if (y >= height-2) break;
		//wow isnt this function name so easy to understand
		mvwaddnwstr(window,y+1,1,str_to_wchar(queue->songs[i].name),width-2);
		if (i == queue->selected_song_index) mvwchgat(window,y+1,1,width-2,A_UNDERLINE | A_DIM,PAIR_DEFAULT,NULL);
		if (i == queue->selected_song_index) cursor_y = y;
		if (i == queue->current_song_index) mvwchgat(window,y+1,1,width-2,A_REVERSE,PAIR_DEFAULT,NULL);
		y++;
	}
	//move cursor to current selection
	wmove(window,cursor_y+1,1);
	wrefresh(window);
}
wchar_t *str_to_wchar(const char *str){
	static wchar_t buffer[4096]; //probs big enough
	//at least the naming scheme here is sensible
	size_t count = mbstowcs(buffer,str,4096);
	if (count == -1) wcscpy(buffer,L"Error");
	return buffer;
}
void playback_status_window_update(WINDOW *window,struct music_queue *queue){
	int width,height;
	getmaxyx(window,height,width);
	//border and such
	werase(window);
	box_set(window,0,0);
	//window "title"
	mvwaddnwstr(window,0,1,L"┤playback status├",width-2);
	//render a spinner
	wchar_t spinner_state;
	if (queue->playback_status == PLAYBACK_PLAYING) spinner_state = L"-\\|/"[time(NULL)%4];
	else spinner_state = L'#';
	mvwaddnwstr(window,1,1,&spinner_state,1);
	//display playing state
	if (width >= 12 &&queue->playback_status == PLAYBACK_PLAYING) mvwaddwstr(window,1,3,L"Playing");
	else if (width >= 12 && queue->playback_status == PLAYBACK_PAUSED) mvwaddwstr(window,1,3,L"Paused");
	else if (width >= 12) mvwaddwstr(window,1,3,L"Stopped");
	//print time remaining
	int seconds_elapsed = Mix_GetMusicPosition(queue->songs[queue->current_song_index].song);
	int seconds_remaining = Mix_MusicDuration(queue->songs[queue->current_song_index].song);
	if (width >= 24) mvwprintw(window,1,12,"%d:%02d / %d:%02d",seconds_elapsed/60,seconds_elapsed%60,seconds_remaining/60,seconds_remaining%60);
	//print artist and song name
	if (height >= 4) mvwaddnwstr(window,2,1,str_to_wchar(Mix_GetMusicTitleTag(queue->songs[queue->current_song_index].song)),width-2);
	if (height >= 5) mvwaddnwstr(window,3,1,str_to_wchar(Mix_GetMusicArtistTag(queue->songs[queue->current_song_index].song)),width-2);
	//refresh
	wrefresh(window);
}
void sigwinch_handler(int sig){
	sigwinch_occured = 1;
}
