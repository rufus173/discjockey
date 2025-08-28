#ifndef _VISUALISER_H
#define _VISUALISER_H

#include <ncurses.h>
#include "queue.h"

void visualiser_callback(void *user_data,uint8_t *stream,int len);
void visualiser_window_update(WINDOW *window,struct music_queue *queue);

#endif
