#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H 

#define MEM_SIZE 1000
#define MAX_LOADED_SCRIPTS 5
#define FRAME_SIZE 3

#ifndef FRAME_STORE_SIZE
#define FRAME_STORE_SIZE 18
#endif

#ifndef VAR_MEM_SIZE
#define VAR_MEM_SIZE 10
#endif

#include "pcb.h"

 
typedef struct {
	char *script_name;
    	int start_index;
    	int length;
    	int ref_count;
    	int *page_table;
    	int num_pages;
} loaded_script;


extern loaded_script loaded_scripts[MAX_LOADED_SCRIPTS];
extern int num_loaded_scripts;


void mem_init();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int mem_load_line(char *line);
char * mem_get_line(int index);
void mem_cleanup_script(int start_index, int end_index);
int mem_load_script(char *script, int *start_index);
int mem_load_from_batch(int *start_index);
int mem_load_script_sharing(char *script, int *start_index);
char *get_instruction(PCB *pcb, int instruction_index);
void unload_script_with_sharing(char *script, int start_index);

//frame store
void frame_store_init();
int frame_store_alloc_frame();
void frame_store_set_line(int frame, int line_in_frame, const char *text);
const char *frame_store_get_line(int frame, int line_in_frame);
void frame_store_free_frame(int frame);
int frame_store_lru_victim(void);
void frame_store_print_frame(int frame);
int frame_store_is_allocated(int frame);
int frame_store_num_frames();
void frame_store_mark_used(int frame);

#endif
