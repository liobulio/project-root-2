#include <stdio.h>

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 18
#endif
#ifndef VAR_MEM_SIZE
#define VAR_MEM_SIZE 10
#endif

#define FRAME_SIZE 3

//variable store
void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);

//frame store
void frame_store_init();
int frame_store_alloc_frames();
void frame_store_set_line(int frame, int line_in_frame, const char *text);
const char *frame_store_get_line(int frame, int line_in_frame);
void frame_store_free_frame(int frame);
int frame_store_lru_victim(void);
void frame_store_print_frame(int frame);
void frame_store_is_allocated();
void frame_store_num_frames();
