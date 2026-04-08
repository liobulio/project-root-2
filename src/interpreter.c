//#define DEBUG 1

#ifdef DEBUG
#   define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#   define debug(...)
// NDEBUG disables asserts
//#   define NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>              // tolower, isdigit
#include <dirent.h>             // scandir
#include <unistd.h>             // chdir
#include <sys/stat.h>           // mkdir
// for run:
#include <sys/types.h>          // pid_t
#include <sys/wait.h>           // waitpid
#include <stdbool.h>

#include "shellmemory.h"
#include "shell.h"
#include "pcb.h"
#include "ready_queue.h"
#include "scheduler.h"

#define POLICY_FCFS 1
#define POLICY_SJF 2
#define POLICY_RR 3
#define POLICY_AGING 4
#define POLICY_RR30 5

ready_queue *my_queue = NULL;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int badcommandMkdir() {
    printf("Bad command: my_mkdir\n");
    return 4;
}

int badcommandCd() {
    printf("Bad command: my_cd\n");
    return 5;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int echo(char *tok);
int ls();
int my_mkdir(char *name);
int touch(char *path);
int cd(char *path);
int source(char *script);
int run(char *args[], int args_size);
int exec(char *args[], int args_size);
int badcommandFileDoesNotExist();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    // these bits of debug output were very helpful for debugging
    // the changes we made to the parser!
    debug("#args: %d\n", args_size);
#ifdef DEBUG
    for (size_t i = 0; i < args_size; ++i) {
        debug("  %ld: %s\n", i, command_args[i]);
    }
#endif

    if (args_size < 1) {
        // This shouldn't be possible but we are defensive programmers.
        fprintf(stderr, "interpreter called with no words?\n");
        exit(1);
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        if (args_size != 1)
            return badcommand();
        return ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "my_touch") == 0) {
        if (args_size != 2)
            return badcommand();
        return touch(command_args[1]);

    } else if (strcmp(command_args[0], "my_cd") == 0) {
        if (args_size != 2)
            return badcommand();
        return cd(command_args[1]);

    } else if (strcmp(command_args[0], "exec") == 0) {
        if (args_size < 3 || args_size > 7)
            return badcommand();
        return exec(&command_args[1], args_size - 1);
        
    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else if (strcmp(command_args[0], "run") == 0) {
        if (args_size < 2)
            return badcommand();
        return run(&command_args[1], args_size - 1);

    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
source SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
	printf("Bye!\n");
	join_threads();
    exit(0);
}

int set(char *var, char *value) {
    mem_set_value(var, value);
    return 0;
}

int print(char *var) {
    char *value = mem_get_value(var);
    if (value) {
        printf("%s\n", value);
        free(value);
    } else {
        printf("Variable does not exist\n");
    }
    return 0;
}

int echo(char *tok) {
    int must_free = 0;
    // is it a var?
    if (tok[0] == '$') {
        tok++;                  // advance pointer, so that tok is now the stuff after '$'
        tok = mem_get_value(tok);
        if (tok == NULL) {
            tok = "";           // must use empty string, can't pass NULL to printf
        } else {
            must_free = 1;
        }
    }

    printf("%s\n", tok);

    // memory management technically optional for this assignment
    if (must_free) free(tok);

    return 0;
}

// We can hide dotfiles in ls using either the filter operand to scandir,
// or by checking the first character ourselves when we go to print
// the names. That would work, and is less code, but this is more robust.
// And this is also better since it won't allocate extra dirents.
int ls_filter(const struct dirent *d) {
    if (d->d_name[0] == '.') return 0;
    return 1;
}

int ls_compare_char(char a, char b) {
    // assumption: a,b are both either digits or letters.
    // If this is not true, the characters will be effectively compared
    // as ASCII when we do the lower_a - lower_b fallback.

    // if both are digits, compare them
    if (isdigit(a) && isdigit(b)) {
        return a - b;
    }
    // if only a is a digit, then b isn't, so a wins.
    if (isdigit(a)) {
        return -1;
    }

    // lowercase both letters so we can compare their alphabetic position.
    char lower_a = tolower(a), lower_b = tolower(b);
    if (lower_a == lower_b) {
        // a and b are the same letter, possibly in different cases.
        // If they are really the same letter, this returns 0.
        // Otherwise, it's negative if A was capital,
        // and positive if B is capital.
        return a - b;
    }

    // Otherwise, compare their alphabetic position by comparing
    // them at a known case.
    return lower_a - lower_b;
}

int ls_compare_str(const char *a, const char *b) {
    // a simple strcmp implementation that uses ls_compare_char.
    // We only check if *a is zero, since if *b is zero earlier,
    // it would've been unequal to *a at that time and we would return.
    // If *b is zero at the same point or later than *a, we'll exit the
    // loop and return the correct value with the last comparison.

    while (*a != '\0') {
        int d = ls_compare_char(*a, *b);
        if (d != 0) return d;
        a++, b++;
    }
    return ls_compare_char(*a, *b);
}

int ls_compare(const struct dirent **a, const struct dirent **b) {
    return ls_compare_str((*a)->d_name, (*b)->d_name);
}

int ls() {
    // straight out of the man page examples for scandir
    // alphasort uses strcoll instead of strcmp,
    // so we have to implement our own comparator to match the ls spec.
    // Note that the test cases weren't very picky about the specified order,
    // so if you just used alphasort with scandir, you should have passed.
    // This was intentional on our part.
    struct dirent **namelist;
    int n;

    n = scandir(".", &namelist, NULL, ls_compare);
    if (n == -1) {
        // something is catastrophically wrong, just give up.
        perror("my_ls couldn't scan the directory");
        return 0;
    }

    for (size_t i = 0; i < n; ++i) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);

    return 0;
}

int str_isalphanum(char *name) {
    for (char c = *name; c != '\0'; c = *++name) {
        if (!(isdigit(c) || isalpha(c))) return 0;
    }
    return 1;
}

int my_mkdir(char *name) {
    int must_free = 0;

    debug("my_mkdir: ->%s<-\n", name);

    if (name[0] == '$') {
        ++name;
        // lookup name
        name = mem_get_value(name);
        debug("  lookup: %s\n", name ? name : "(NULL)");
        if (name) {
            // name exists, should free whatever we got
            must_free = 1;
        }
    }
    if (!name || !str_isalphanum(name)) {
        // either name doesn't exist, or isn't valid, error.
        if (must_free) free(name);
        return badcommandMkdir();
    }
    // at this point name is definitely OK

    // 0777 means "777 in octal," aka 511. This value means
    // "give the new folder all permissions that we can."
    int result = mkdir(name, 0777);

    if (result) {
        // description doesn't specify what to do in this case,
        // (including if the directory already exists)
        // so we just give an error message on stderr and ignore it.
        perror("Something went wrong in my_mkdir");
    }

    if (must_free) free(name);
    return 0;
}

int touch(char *path) {
    // we're told we can assume this.
    assert(str_isalphanum(path));
    // if things go wrong, just ignore it.
    FILE *f = fopen(path, "a");
    fclose(f);
    return 0;
}

int cd(char *path) {
    // we're told we can assume this.
    assert(str_isalphanum(path));

    int result = chdir(path);
    if (result) {
        // chdir can fail for several reasons, but the only one we need
        // to handle here for the spec is the ENOENT reason,
        // aka Error NO ENTry -- the directory doesn't exist.
        // Since that's the only one we have to handle, we'll just assume
        // that that's what happened.
        // Alternatively, you can check if the directory exists
        // explicitly first using `stat`. However it is often better to
        // simply try to use a filesystem resource and then recover when
        // you can't, rather than trying to validate first. If you validate
        // first while two users are on the system, there's a race condition!
        return badcommandCd();
    }
    return 0;
}

/*int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }


    int start_index = -1;
    int end_index = -1;
    int first_line = 1;
    
    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {

        int current_index = mem_load_line(line);

        if (current_index == -1) {
            printf("Error: Shell memory full!\n");
            fclose(p);
            return 1;
        } 
        
        errCode = parseInput(line);     // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
}*/

int run(char *args[], int arg_size) {
    // copy the args into a new NULL-terminated array.
    char **adj_args = calloc(arg_size + 1, sizeof(char *));
    for (int i = 0; i < arg_size; ++i) {
        adj_args[i] = args[i];
    }

    // always flush output streams before forking.
    fflush(stdout);
    // attempt to fork the shell
    pid_t pid = fork();
    if (pid < 0) {
        // fork failed. Report the error and move on.
        perror("fork() failed");
        return 1;
    } else if (pid == 0) {
        // we are the new child process.
        execvp(adj_args[0], adj_args);
        perror("exec failed");
        // The parent and child are sharing stdin, and according to
        // a part of the glibc documentation that you are **not**
        // expected to know for this course, a shared input handle
        // should be fflushed (if it is needed) or closed
        // (if it is not). Handling this exec error case is not even
        // necessary, but let's do it right.
        // (Failure to do this can result in the parent process
        // reading the remaining input twice in batch mode.)
        fclose(stdin);
        exit(1);
    } else {
        // we are the parent process.
        waitpid(pid, NULL, 0);
    }

    return 0;
}


int source(char *script) {
    char *args[2];
    args[0] = script;
    args[1] = "RR";
    return exec(args, 2);
}

// exec function
int exec(char *args[], int args_size) {

    // input validation
    if(args_size < 2 || args_size > 6){
        printf("Error: Need 1-3 scripts + policy (total %d arguments)\n", args_size);
        return -1;
    }

	// check variables
    char *policy_str;
    int policy_code = -1;
    int background = 0;
    int multithread = 0;
    int num_scripts;
    // extract and validate the policy

	// check MT and # and make sure no out of bound
	if(args_size >= 3 && (strcmp(args[args_size - 1], "MT") == 0) && (strcmp(args[args_size - 2], "#") == 0)){
		multithread = 1;
		background = 1;
		policy_str = args[args_size - 3];
        num_scripts = args_size - 3;
	}
	// if multi thread
 	else if ((strcmp(args[args_size - 1], "MT")) == 0){
		multithread = 1;
		policy_str = args[args_size - 2];
        num_scripts = args_size - 2;
	}
	// if background
    else if((strcmp(args[args_size - 1], "#")) == 0){
        background = 1;
        policy_str = args[args_size - 2];
        num_scripts = args_size - 2;
    }
	// default
    else {
        policy_str = args[args_size - 1];
        num_scripts = args_size - 1;
    }

	// match policy
    if(strcmp(policy_str, "FCFS") == 0) policy_code = POLICY_FCFS;
    else if(strcmp(policy_str, "SJF") == 0) policy_code = POLICY_SJF;
    else if(strcmp(policy_str, "RR") == 0) policy_code = POLICY_RR;
    else if(strcmp(policy_str, "AGING") == 0) policy_code = POLICY_AGING;
    else if(strcmp(policy_str, "RR30") == 0) policy_code = POLICY_RR30;
    else {
        printf("Error: Unknown policy '%s'. Use FCFS, SJF, RR, RR30 or AGING.\n", policy_str);
        return -1;
    }


    // load scripts
    int starts[4] = {0};    // initialize to 0
    int lengths[4] = {0};
    int loaded = 0;
    bool error = false;

    // create PCB for batch (rest of code)
    if (background == 1) {
        int batch_start = 0;
        int length_batch = mem_load_from_batch(&batch_start);

        if(length_batch == -1) {
            printf("Error while loading script\n");
            error = true;
        }

	    else {
	     starts[0] = batch_start;
             lengths[0] = length_batch;

             loaded ++;

             for(int i = 0; i < num_scripts; i++) {
            	lengths[i + 1] = mem_load_script_sharing(args[i], &starts[i + 1]);

             	if(lengths[i + 1] == -1) {
                	printf("Error while loading script '%s'\n", args[i]);
                	error = true;
                	break;
             	}
             	loaded++;
	     }
         }
    } 
    else{
		//background != 1, foreground mode
		if (my_queue == NULL) my_queue = create_queue();
	
		for(int i = 0; i < num_scripts; i++) {                    
	   	 	PCB *new_pcb = make_pcb(i + 1, 0, 0, args[i]);		
	    	if (load_script_with_sharing_paging(args[i], new_pcb) == -1) {
        		printf("Bad command: File not found\n");
        		free(new_pcb->script_name); free(new_pcb);
        		return -1;
    		}
    		enqueue_fifo(my_queue, new_pcb);  // (or sorted for SJF/AGING)	
        }
    }

    // clean up on error
    if (error) {
        for (int j = 0; j < loaded; j++) {
            mem_cleanup_script(starts[j], starts[j] + lengths[j] - 1);
        }
        return -1;
    }

    // create queue if needed
    if (my_queue == NULL) {
        my_queue = create_queue();
    }


    /* create PCBs and enqueue them
    if (background == 1) {

		// deal with first pcb and enqueue it with fifo
		PCB *new_pcb = make_pcb(1, starts[0], lengths[0], args[0]);
		enqueue_fifo(my_queue, new_pcb);

        for (int i = 0; i < num_scripts; i++) {
            // PID is i+1 (1, 2, 3...)
            PCB *new_pcb = make_pcb(i+2, starts[i+1], lengths[i+1], args[i+1]);

			// euqueue policy
            if (policy_code == POLICY_FCFS || policy_code == POLICY_RR || policy_code == POLICY_RR30 ){
                enqueue_fifo(my_queue, new_pcb);
            }
			else if (policy_code == POLICY_AGING) {
                enqueue_sorted_by_score_with_batch(my_queue, new_pcb);
            }
			else if (policy_code == POLICY_SJF){
				enqueue_sorted_by_length_with_batch(my_queue, new_pcb);
			}

        }
    }
    else {
        for (int i = 0; i < num_scripts; i++) {
            // PID is i+1 (1, 2, 3...)
            PCB *new_pcb = make_pcb(i + 1, starts[i], lengths[i], args[i]);

			// enqueue policy
            if (policy_code == POLICY_SJF) {
                enqueue_sorted_by_length(my_queue, new_pcb);
            } else if (policy_code == POLICY_FCFS || policy_code == POLICY_RR || policy_code == POLICY_RR30 ){
                enqueue_fifo(my_queue, new_pcb);
            } else if ((policy_code == POLICY_AGING)) {
                enqueue_sorted_by_score(my_queue, new_pcb);
            }

        }
    } */

    if (multithread == 1) {
        enable_multithreading(); 
    } 
    return scheduler(policy_code);
}

void frame_store_clear_owner(PCB *pcb) {
    int nf = FRAME_STORE_SIZE / FRAME_SIZE;
    for (int f = 0; f < nf; f++) {
        if (fmeta[f].pcb_of_frame == pcb) {
            fmeta[f].pcb_of_frame = NULL;
            fmeta[f].page = -1;
        }
    }
}
