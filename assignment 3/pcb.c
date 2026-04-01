#include <stdlib.h>
#include "pcb.h"

PCB * make_pcb(int pid, int start, int length, char *script_name) {
    PCB * new_pcb = malloc(sizeof(PCB));

    if (new_pcb == NULL){
      return NULL;
    }
  
    new_pcb->PID = pid;
    new_pcb->start_position= start;
    new_pcb->length = length;
    new_pcb->pc_instruction_index = start;
    new_pcb->job_length_score = length; 
    new_pcb->next = NULL;
    new_pcb->script_name = strdup(script_name);
    return new_pcb;
}
