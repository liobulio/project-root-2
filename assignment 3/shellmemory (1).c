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
