#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "shellmemory.h"

struct memory_struct {
    char *var;
    char *value;
};

typedef struct {
    char *script_name;
    int start_index;
    int length;
    int ref_count;
} loaded_script;

// save information of loadedscripte
loaded_script loaded_scripts[MAX_LOADED_SCRIPTS];
int num_loaded_scripts = 0;

struct memory_struct shellmemory[MEM_SIZE];
char *shell_code[MEM_SIZE]; // reserve space for script lines

// Helper functions
int match(char *model, char *var) {
    int i, len = strlen(var), matchCount = 0;
    for (i = 0; i < len; i++) {
        if (model[i] == var[i])
            matchCount++;
    }
    if (matchCount == len) {
        return 1;
    } else
        return 0;
}

// Shell memory functions

void mem_init() {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
        shell_code[i] = NULL;
    }
    for (i = 0; i < 5; i++) {
        loaded_scripts[i].script_name = NULL;
        loaded_scripts[i].start_index = -1;
        loaded_scripts[i].length = 0;
        loaded_scripts[i].ref_count = 0;
    }
    num_loaded_scripts = 0;
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    return;
}

//get value based on input key
char *mem_get_value(char *var_in) {
    int i;

    for (i = 0; i < MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return NULL;
}

//return the index where the line got loaded
int mem_load_line(char *line) {
    int i;
    for (i = 0; i < MEM_SIZE; i++) {
        if (shell_code[i] == NULL) {
            shell_code[i] = strdup(line);
            return i; //return the index so PCB knows where to start
        }
    }
    return -1; //memory full
}

//return the line of script located at that index
char * mem_get_line(int index) {
    if (index < 0 || index >= MEM_SIZE) return NULL;
    return shell_code[index];
}

//clean up the shell_code array 
void mem_cleanup_script(int start_index, int end_index) {
    for (int i = start_index; i <= end_index; i++) {
        if (shell_code[i] != NULL) {
            free(shell_code[i]);
            shell_code[i] = NULL;
        }
    }
}

// this check first if input prog exist and cound number of line
int mem_load_script(char *script, int *start_index) {

	int first_index = -1;
    int num_line = 0;

	// check if file exist or not
	FILE *f = fopen(script, "r");
	if (f == NULL) {
		return -1;
	}

	int input_limit = 200;
	char buffer[input_limit];

	// count number of line in script
	while (fgets(buffer, input_limit, f)) {

		// if empty line then means end of file
        int index = mem_load_line(buffer);
        if (index == -1) {
            fclose(f);
            return -1;
        }

		// set first_index to current index
		if (first_index == -1) {
            first_index = index;
        }

        num_line = num_line + 1;
    }
	fclose(f);
	// set start index of file to current index which is start+
	*start_index = first_index;
	return num_line;
}

// NEW PUBLIC function - use this everywhere instead
int mem_load_script_sharing(char *script, int *start_index) {
    // Check if already loaded
    for(int i = 0; i < num_loaded_scripts; i++) {
        if(loaded_scripts[i].script_name != NULL &&
           strcmp(loaded_scripts[i].script_name, script) == 0) {
            // Found existing script - share it!
            *start_index = loaded_scripts[i].start_index;
            loaded_scripts[i].ref_count++;
            return loaded_scripts[i].length;
           }
    }

    int length = mem_load_script(script, start_index);

    if(length != -1) {
        // Add to tracking table
        if(num_loaded_scripts < MAX_LOADED_SCRIPTS) {
            loaded_scripts[num_loaded_scripts].script_name = strdup(script);
            loaded_scripts[num_loaded_scripts].start_index = *start_index;
            loaded_scripts[num_loaded_scripts].length = length;
            loaded_scripts[num_loaded_scripts].ref_count = 1;
            num_loaded_scripts++;
        }
    }

    return length;
}

// don't free loaded_script too fast, need to unload it until none of script share same memory
void unload_script_with_sharing(char *script, int start_index) {

    for(int i = 0; i < num_loaded_scripts; i++) {
        if(loaded_scripts[i].script_name != NULL &&
           strcmp(loaded_scripts[i].script_name, script) == 0 &&
           loaded_scripts[i].start_index == start_index) {

            loaded_scripts[i].ref_count--;

            if(loaded_scripts[i].ref_count == 0) {
                // Free the actual memory
                mem_cleanup_script(start_index,
                                  start_index + loaded_scripts[i].length - 1);
                free(loaded_scripts[i].script_name);
                loaded_scripts[i].script_name = NULL;

                // Optional: compact the array (not necessary for small MAX)
            }
            break;
           }
    }
}

// this get the rest of input and give the count
int mem_load_from_batch(int *start_index) {

	int first_index = -1;
    int num_line = 0;

	int input_limit = 200;
	char buffer[input_limit];

	// count number of line in script
	while (fgets(buffer, input_limit, stdin) != NULL) {

		// if empty line then means end of file
        int index = mem_load_line(buffer);
        if (index == -1) {
			// clean up memory
            if (first_index != -1) {
				mem_cleanup_script(first_index, first_index + num_line - 1);
            }
            return -1;
        }

		// set first_index to current index
		if (first_index == -1) {
            first_index = index;
        }

        num_line = num_line + 1;
    }
	// set start index of file to current index which is start+
	*start_index = first_index;
	return num_line;
}
// same as mem_cleanup_memory but only clean script when no share memory required
void unload_script_with_sharing(char *script, int start_index) {

    for(int i = 0; i < num_loaded_scripts; i++) {
        if(loaded_scripts[i].script_name != NULL &&
           strcmp(loaded_scripts[i].script_name, script) == 0 &&
           loaded_scripts[i].start_index == start_index) {

            loaded_scripts[i].ref_count--;

            if(loaded_scripts[i].ref_count == 0) {
                // Free the actual memory
                mem_cleanup_script(start_index,
                                  start_index + loaded_scripts[i].length - 1);
                free(loaded_scripts[i].script_name);
                loaded_scripts[i].script_name = NULL;

                // Optional: compact the array (not necessary for small MAX)
            }
            break;
           }
    }
}

// -----------Frame Store-----------

struct frame_slot {
    char *line;
};

struct frame_meta {
    int allocated;
    unsigned long lru_clock; // later for LRU policy
};


// FRAME_SIZE defined in shellmemory.h
static struct frame_slot fstore[FRAME_STORE_SIZE]; // one slot = one line, an array of lines
static struct frame_meta fmeta[FRAME_STORE_SIZE / FRAME_SIZE];  // metadata about the frames, an array of frames

static unsigned long g_clock = 0;

void frame_store_init() {
    // free each line and reset the pointer to zero
    for (int i = 0; i < FRAME_STORE_SIZE; i++) {
        free(fstore[i].line);
        fstore[i].line = NULL;
    }

    int nf = FRAME_STORE_SIZE / FRAME_SIZE;
    for (int j = 0; j < nf; j++) {
        fmeta[j].allocated = 0; // mark as free and available to load more frames
        fmeta[j].lru_clock = 0;
    }
    g_clock = 0;
}

// find the first available frame and returns the index of that frame
int frame_store_alloc_frame() {
    int nf = FRAME_STORE_SIZE / FRAME_SIZE;
    for (int f = 0; f < nf; f++) {
        if (!fmeta[f].allocated) {
            fmeta[f].allocated = 1;
            fmeta[f].lru_clock = ++g_clock;
            return f;
        }
    }
    return -1; // frame store is full
}

void frame_store_set_line(int frame, int line_in_frame, const char *text) {
    int slot = frame * FRAME_SIZE + line_in_frame;
    free(fstore[slot].line);
    fstore[slot].line = text ? strdup(text) : NULL;
}

// frame: 0-2, FRAME_SIZE: 3, line_in_frame: 0-2
const char *frame_store_get_line(int frame, int line_in_frame) {
    int slot = frame * FRAME_SIZE + line_in_frame;
    return fstore[slot].line;
}

void frame_store_free_frame(int frame) {
    fmeta[frame].allocated = 0;
    fmeta[frame].lru_clock = 0;
}


// scanning for the oldest frame
// ignores empty frames, and search for the frame having the smallest lru_clock
// returns the index of the oldest frame/victim
int frame_store_lru_victim() {
    int nf = FRAME_STORE_SIZE / FRAME_SIZE;
    int victim = -1;
    unsigned long oldest = (unsigned long)(-1);
    for (int i = 0; i < nf; i++) {
        if (fmeta[i].allocated && fmeta[i].lru_clock < oldest) {
            oldest = fmeta[i].lru_clock;
            victim = i;
        }
    }
    return victim;
}

// print the content of  the victim page,
void frame_store_print_frame(int frame) {
    printf("\n");
    for (int i = 0; i < FRAME_SIZE; i++) {
        const char *line = frame_store_get_line(frame, i);
        if (line) {
            printf("%s", line);
            /* ensure newline */
            size_t len = strlen(line);
            if (len == 0 || line[len-1] != '\n') printf("\n");
        }
    }
    printf("\n");
}

int frame_store_is_allocated(int frame) {
   return fmeta[frame].allocated;
}
int frame_store_num_frames() {
     return FRAME_STORE_SIZE / FRAME_SIZE;
}

void frame_store_mark_used(int frame) {
    fmeta[frame].lru_clock = ++g_clock;
}
