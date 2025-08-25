#ifndef _QUEUE_H
#define _QUEUE_H

#include <SDL_mixer.h>

struct music_queue {
	Mix_Music **songs;
	int song_count;
	int current_song_index;
};

int queue_load(char **files, int file_count, struct music_queue *queue);
void queue_free(struct music_queue *queue);
void shuffle_queue(struct music_queue *queue);
int queue_start(struct music_queue *queue);
int queue_play(struct music_queue *queue);
int queue_next(struct music_queue *queue);
int queue_prev(struct music_queue *queue);

#endif
