#include "input.h"
#include "background.h"
#include <signal.h>

char* read_input() {
    char* line = NULL;
    size_t bufsize = 0;
    ssize_t characters = getline(&line, &bufsize, stdin);
    
    if (characters == -1) {
        if (feof(stdin)) {
            // Ctrl-D (EOF) detected - Part E.3 requirement
            printf("logout\n");
            
            // Send SIGKILL to all child processes
            for (int i = 0; i < job_count; i++) {
                kill(jobs[i].pid, SIGKILL);
            }
            
            exit(0);
        } else {
            perror("getline");
            exit(EXIT_FAILURE);
        }
    }
    
    // Remove trailing newline
    if (line[characters - 1] == '\n') {
        line[characters - 1] = '\0';
    }
    
    return line;
}
