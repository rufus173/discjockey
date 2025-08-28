#include <SDL_mixer.h>
#include <stdint.h>
#include "queue.h"
#include <ncurses.h>

#define VISUALISER_DATA_SIZE 50

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

//the height of the bars
uint8_t visualiser_data[VISUALISER_DATA_SIZE];
uint8_t visualiser_last_heights[VISUALISER_DATA_SIZE];

void visualiser_callback(void *user_data,uint8_t *stream,int len){
	//--- i have no idea how the data is stored in stream ---
	//so im improvising. evenly distribute all of the uint8_t pointers over
	//visualiser_data, then grab as much as we need for the bars, and use each
	//value / 255 as a percentage height for the bars
	//====== distribute stream over visualiser_data ======
	for (int i = 0; i < VISUALISER_DATA_SIZE; i++){
		// (        i+1                 )
		// ( -------------------- x len ) - 1
		// ( VISUALISER_DATA_SIZE       )
		int stream_index = (int)MAX(((((float)i + 1.0)/VISUALISER_DATA_SIZE)*len)-1,0);
		//take rolling average for smoothing
		uint8_t old_val = visualiser_data[i];
		visualiser_data[i] = ((float)stream[stream_index]+(float)old_val)/2;
	}
}

void visualiser_window_update(WINDOW *window,struct music_queue *queue){
	int width,height;
	getmaxyx(window,height,width);
	werase(window);
	//box_set(window,0,0);
	//foreach bar
	for (int i = 0; i < width; i++){
		int visualiser_data_index = (int)MAX(((((float)i+1)/width)*VISUALISER_DATA_SIZE)-1,0);
		int bar_height = visualiser_data[visualiser_data_index];
		int average_height = ((float)bar_height+(float)visualiser_last_heights[visualiser_data_index])/2;
		for (int y = (height-1)*((float)average_height/255); y > 0; y--){
			mvwaddnwstr(window,height-y,i,L"█",1);
		}
		visualiser_last_heights[visualiser_data_index] = average_height;
	}
	//refresh
	wrefresh(window);
}
