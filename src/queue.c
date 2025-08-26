#define _GNU_SOURCE
#include <time.h>
#include <libgen.h>
#include "queue.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

static void swap_ptr(void **a, void **b){
	void *temp = *a;
	*a = *b;
	*b = temp;
}

int scandir_filter(const struct dirent *ent){
	return strcmp(ent->d_name,".") && strcmp(ent->d_name,"..");
}
int queue_load(char **files, int file_count, struct music_queue *queue){
	for (int i = 0; i < file_count; i++){
		printf("loading %s...\n",files[i]);
		//====== exists? ======
		struct stat statbuf;
		if (stat(files[i],&statbuf) < 0){
			fprintf(stderr,"Could not stat %s\n",files[i]);
			perror("stat");
			continue;
		}
		//====== file or dir? ======
		switch (statbuf.st_mode & S_IFMT){
			case S_IFREG:
			//if file just add it to the list
			Mix_Music *song = Mix_LoadMUS(files[i]);
			if (song != NULL){
				queue->song_count++;
				queue->songs = realloc(queue->songs,sizeof(Mix_Music *)*queue->song_count);
				queue->songs[queue->song_count-1] = song;
				queue->song_names = realloc(queue->song_names,sizeof(char *)*queue->song_count);
				char *filename_cpy = strdup(files[i]);
				queue->song_names[queue->song_count-1] = strdup(basename(filename_cpy));
				free(filename_cpy);
			}
			break;
			case S_IFDIR:
			//if dir, call recursively with the contents of the directory
			struct dirent **dir_list;
			char **name_list = NULL;
			int count = scandir(files[i],&dir_list,scandir_filter,alphasort);
			if (count > 0){
				name_list = malloc(sizeof(char *)*count);
				for (int j = 0; j < count; j++){
					asprintf(name_list+j,"%s/%s",files[i],dir_list[j]->d_name);
					free(dir_list[j]);
				}
				free(dir_list);
				queue_load(name_list,count,queue);
				for (;count > 0; count--) free(name_list[count-1]);
				free(name_list);
			}
			break;
		}
	}
	return queue->song_count;
}
void queue_free(struct music_queue *queue){
	for (;queue->song_count > 0; queue->song_count--){
		Mix_FreeMusic(queue->songs[queue->song_count-1]);
		free(queue->song_names[queue->song_count-1]);
	}
	free(queue->song_names);
	free(queue->songs);
}
void queue_shuffle(struct music_queue *queue){
	printf("shuffling...\n");
	//====== fisher-yates in place shuffle ======
	for (int i = queue->song_count-1; i >= 0; i--){
		int pos = random() % (i+1);
		//swap
		swap_ptr((void **)(queue->songs+pos),(void **)(queue->songs+i));
		swap_ptr((void **)(queue->song_names+pos),(void **)(queue->song_names+i));

	}
}
