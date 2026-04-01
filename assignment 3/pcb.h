#ifndef PCB_H
#define PCB_H

typedef struct PCB {

    int PID;                   //process id
    int start_position;        //start index of code in shellmemory 
    int length;
    int pc_instruction_index;  //program counter
    int job_length_score;
    char *script_name;
    struct PCB *next;
} PCB;

//a pcb constructor so we don't use malloc and set the fields every time
PCB *make_pcb(int pid, int start, int length);


#endif
