#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "shellmemory.h"


#define true 1
#define false 0

/* since we don't contiguously olad lins any more
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



// for exec memory

struct program_line {
    int allocated; // for sanity-checking
    char *line;
};

struct program_line linememory[MEM_SIZE];
size_t next_free_line = 0;

void reset_linememory_allocator() {
    next_free_line = 0;
    assert_linememory_is_empty();
}

void assert_linememory_is_empty() {
    for (size_t i = 0; i < MEM_SIZE; ++i) {
        assert(!linememory[i].allocated);
        assert(linememory[i].line == NULL);
    }
}

void init_linemem() {
    for (size_t i = 0; i < MEM_SIZE; ++i) {
        linememory[i].allocated = false;
        linememory[i].line = NULL;
    }
}

size_t allocate_line(const char *line) {
    if (next_free_line >= MEM_SIZE) {
        // out of memory!
        return (size_t)(-1);
    }
    size_t index = next_free_line++;
    assert(!linememory[index].allocated);

    linememory[index].allocated = true;
    linememory[index].line = strdup(line);
    return index;
}

void free_line(size_t index) {
    free(linememory[index].line);
    linememory[index].allocated = false;
    linememory[index].line = NULL;
}

const char *get_line(size_t index) {
    assert(linememory[index].allocated);
    return linememory[index].line;
}
*/
// Shell memory functions --> we treat the old shell memory struct as variable store
// -----------Variable Store-----------

struct memory_struct { // block or line
    char *var;
    char *value;
};

struct memory_struct shellmemory[VAR_MEM_SIZE];

void mem_init() {
    int i;
    for (i = 0; i < VAR_MEM_SIZE; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;

    for (i = 0; i < VAR_MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    //Value does not exist, need to find a free spot.
    for (i = 0; i < VAR_MEM_SIZE; i++) {
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

    for (i = 0; i < VAR_MEM_SIZE; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            return strdup(shellmemory[i].value);
        }
    }
    return NULL;
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
    
    int nf = FRAME_STORE_SIZE / FRAME_SIZE
    for (int j = 0; j < nf; j++) {
        fmeta[j].allocated = 0; // mark as free and available to load more frames
        fmeta[j].lru_clock = 0;
    }
    g_clock = 0;
}

// find the first available frame and returns the index of that frame
int frame_store_alloc_frame() {
    int nf = FRAME_STORE_SIZE / FRAME_SIZE
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
    int nf = FRAME_STORE_SIZE / FRAME_SIZE
    int victim = -1;
    unsigned long oldest = (unsigned long)(-1);
    for (int i = 0; i < nf; i++) {
        if (fmeta[i].allocated && fmeta[f].lru_clock < oldest) {
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


