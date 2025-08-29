#ifndef _QUEUE_H
#define _QUEUE_H

#include <SDL_mixer.h>

struct song {
	Mix_Music *song;
	char *name;
	char *path;
};
enum playback_status {
	PLAYBACK_STOPPED = 0,
	PLAYBACK_PLAYING,
	PLAYBACK_PAUSED,
};
struct music_queue {
	//Mix_Music **songs;
	//char **song_names;
	struct song *songs;
	int song_count;
	int current_song_index; //the song thats playing
	int selected_song_index; //where the cursor is
	enum playback_status playback_status;
	int repeat; //0 for off 1 for one and 2 for all
};

int queue_load(char **files, int file_count, struct music_queue *queue);
void queue_free(struct music_queue *queue);
void queue_shuffle(struct music_queue *queue);
int queue_play(struct music_queue *queue);
int queue_pause_resume(struct music_queue *queue);
int queue_pause(struct music_queue *queue);
int queue_resume(struct music_queue *queue);
int queue_next(struct music_queue *queue);
int queue_prev(struct music_queue *queue);
int queue_repeat_song(struct music_queue *queue);
int queue_restart(struct music_queue *queue);

#endif
