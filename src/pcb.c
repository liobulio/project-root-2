#include <stdlib.h>
#include <string.h>
#include "pcb.h"

PCB * make_pcb(int pid, int start, int length, char *script_name) {
    PCB * new_pcb = malloc(sizeof(PCB));

    if (new_pcb == NULL){
        return NULL;
    }

    new_pcb->PID = pid;
    new_pcb->start_position = start;
    new_pcb->length = length;
    new_pcb->pc_instruction_index = 0;
    new_pcb->job_length_score = length;

    new_pcb->script_name = strdup(script_name);

    new_pcb->page_table = NULL;
    new_pcb->num_pages = 0;
    new_pcb->total_instructions = length;

    new_pcb->next = NULL;

    return new_pcb;
}
