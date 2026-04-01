#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H 

#define MEM_SIZE 1000
#define MAX_LOADED_SCRIPTS 5

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

#endif
