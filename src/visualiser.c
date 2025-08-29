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
//like whats the max volume outa 255. essentialy scales bar height
//you might have to adjust yourself
uint8_t visualiser_data_max_cap = 100;

void visualiser_callback(void *user_data,uint8_t *raw_stream,int len){
	//--- i have no idea how the data is stored in stream ---
	//so im improvising. evenly distribute all of the uint8_t pointers over
	//visualiser_data, then grab as much as we need for the bars, and use each
	//value / 255 as a percentage height for the bars
	//--- update ---
	// len is number of bytes passed
	// uses 2 chanels, pcm, data stored in signed 16 bit slices
	// i think len is the number of samples?
	//  left right left right ...
	// [0,   0,    0,   0,    ...]
	int16_t *stream = (int16_t *)raw_stream;
	int sample_count = len/sizeof(int16_t); //2 samples per thing due to stereo
	//copy the stream
	int16_t *stream_copy = malloc(len);
	memcpy(stream_copy,stream,len);
	//average left and right:
	//average stored in both channel
	for (int i = 0; i < sample_count; i += 2){
		//                 cast to stop overflow
		stream_copy[i] = ((int32_t)stream_copy[i]+(int32_t)stream_copy[i+1])/2;
		stream_copy[i+1] = stream_copy[i];
	}
	//====== distribute stream over visualiser_data ======
	for (int i = 0; i < VISUALISER_DATA_SIZE; i++){
		// (        i+1                 )
		// ( -------------------- x len ) - 1
		// ( VISUALISER_DATA_SIZE       )
		int stream_index = (int)MAX(((((float)i + 1.0)/VISUALISER_DATA_SIZE)*sample_count)-1,0);
		//take rolling average for smoothing
		uint8_t old_val = visualiser_data[i];
		uint8_t new_val = ((float)abs(stream[stream_index])/INT16_MAX)*UINT8_MAX;
		visualiser_data[i] = ((float)new_val+(float)old_val)/2;
	}
	free(stream_copy);
}

//====== dont ask me how this works even i dont know ======
void visualiser_window_update(WINDOW *window,struct music_queue *queue){
	int width,height;
	getmaxyx(window,height,width);
	werase(window);
	//auto adjustment
	int total_bar_display_height = 0;
	//foreach bar
	for (int i = 0; i < width; i++){
		int visualiser_data_index = (int)MAX(((((float)i+1)/width)*VISUALISER_DATA_SIZE)-1,0);
		//0-255
		//average of a bar and its 2 neighbors
		float bar_height = visualiser_data[visualiser_data_index];
		float bar_prev_height = visualiser_data[MAX(0,visualiser_data_index-1)];
		float bar_next_height = visualiser_data[MIN(VISUALISER_DATA_SIZE-1,visualiser_data_index+1)];
		int average_height = (((bar_height+bar_prev_height+bar_next_height)/3)+(float)visualiser_last_heights[visualiser_data_index])/2;
		//scaled to the screen size
		int bar_display_height = (height-1)*MIN(1,(float)average_height/visualiser_data_max_cap);
		total_bar_display_height += bar_display_height;
		for (int y = bar_display_height; y > 0; y--){
			mvwaddnwstr(window,height-y,i,L"█",1);
		}
		visualiser_last_heights[visualiser_data_index] = average_height;
	}
	//auto adjustment
	//int average_bar_display_height = total_bar_display_height / width;
	//int virtual_cap = MIN(UINT8_MAX,2.5*(float)average_bar_display_height);
	//if (visualiser_data_max_cap > virtual_cap) visualiser_data_max_cap--;
	//if (visualiser_data_max_cap < virtual_cap) visualiser_data_max_cap++;
	//refresh
	wrefresh(window);
}
