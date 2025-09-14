#include "background.h"
#include <signal.h>
#include <string.h>
#include <ctype.h>

// Global variables for job control
bg_process_t* jobs = NULL;
int job_count = 0;
int max_jobs = 0;
pid_t foreground_pid = 0;

static int next_job_number = 1;

int get_next_job_number() {
    return next_job_number++;
}

void expand_jobs_array() {
    if (max_jobs == 0) {
        max_jobs = 10;
        jobs = malloc(max_jobs * sizeof(bg_process_t));
    } else {
        max_jobs *= 2;
        jobs = realloc(jobs, max_jobs * sizeof(bg_process_t));
    }
}

void remove_job(int index) {
    if (index < 0 || index >= job_count) return;
    
    free(jobs[index].command);
    
    // Shift remaining jobs
    for (int i = index; i < job_count - 1; i++) {
        jobs[i] = jobs[i + 1];
    }
    job_count--;
}

int find_job_by_pid(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

int find_job_by_number(int job_number) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_number == job_number) {
            return i;
        }
    }
    return -1;
}

// ############## LLM Generated Code Begins ##############
void check_background_processes() {
    int status;
    pid_t pid;
    
    // Check for completed background processes
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        int job_index = find_job_by_pid(pid);
        if (job_index != -1) {
            if (WIFEXITED(status)) {

                printf("%s with pid %d exited normally\n", jobs[job_index].command, jobs[job_index].pid);
                remove_job(job_index);
            } else if (WIFSIGNALED(status)) {
                printf("%s with pid %d exited abnormally\n", jobs[job_index].command, jobs[job_index].pid);
                remove_job(job_index);
            } else if (WIFSTOPPED(status)) {
                jobs[job_index].state = JOB_STOPPED;
                printf("[%d] Stopped %s\n", jobs[job_index].job_number, jobs[job_index].command);
            }
        }
    }
}
// ############## LLM Generated Code Ends ##############

void handle_background_processes() {
    check_background_processes();
}

void add_background_process(pid_t pid, char* command) {
    if (job_count >= max_jobs) {
        expand_jobs_array();
    }
    
    jobs[job_count].pid = pid;
    jobs[job_count].command = strdup(command);
    jobs[job_count].job_number = get_next_job_number();
    jobs[job_count].state = JOB_RUNNING;
    
    printf("[%d] %d\n", jobs[job_count].job_number, pid);
    job_count++;
}

void add_stopped_process(pid_t pid, char* command) {
    if (job_count >= max_jobs) {
        expand_jobs_array();
    }
    
    jobs[job_count].pid = pid;
    jobs[job_count].command = strdup(command);
    jobs[job_count].job_number = get_next_job_number();
    jobs[job_count].state = JOB_STOPPED;
    
    printf("[%d] Stopped %s\n", jobs[job_count].job_number, command);
    job_count++;
}

// Comparison function for sorting jobs by command name
int compare_jobs(const void* a, const void* b) {
    const bg_process_t* job_a = (const bg_process_t*)a;
    const bg_process_t* job_b = (const bg_process_t*)b;
    return strcmp(job_a->command, job_b->command);
}

// E.1: activities command
int cmd_activities(char** args) {
    (void)args;
    
    if (job_count == 0) {
        return 1;
    }
    
    bg_process_t* sorted_jobs = malloc(job_count * sizeof(bg_process_t));
    for (int i = 0; i < job_count; i++) {
        sorted_jobs[i] = jobs[i];
    }
    
    qsort(sorted_jobs, job_count, sizeof(bg_process_t), compare_jobs);
    
    // Print sorted jobs
    for (int i = 0; i < job_count; i++) {
        const char* state_str = (sorted_jobs[i].state == JOB_RUNNING) ? "Running" : "Stopped";
        printf("[%d] : %s - %s\n", sorted_jobs[i].pid, sorted_jobs[i].command, state_str);
    }
    
    free(sorted_jobs);
    return 1;
}

// E.2: ping command
int cmd_ping(char** args) {
    if (args[1] == NULL || args[2] == NULL) {
        printf("Invalid syntax!\n");
        return 1;
    }
    
    // Parse PID
    char* endptr;
    long pid_long = strtol(args[1], &endptr, 10);
    if (*endptr != '\0' || pid_long <= 0) {
        printf("Invalid syntax!\n");
        return 1;
    }
    pid_t pid = (pid_t)pid_long;
    
    long signal_long = strtol(args[2], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid syntax!\n");
        return 1;
    }
    
    int signal_number = (int)signal_long;
    int actual_signal = signal_number % 32;
    
    if (kill(pid, actual_signal) == -1) {
        printf("No such process found\n");
    } else {
        printf("Sent signal %d to process with pid %d\n", signal_number, pid);
    }
    
    return 1;
}

// E.4: fg command
int cmd_fg(char** args) {
    int job_index = -1;
    
    if (args[1] == NULL) {
        if (job_count == 0) {
            printf("No such job\n");
            return 1;
        }
        job_index = job_count - 1;
    } else {
        char* endptr;
        long job_num = strtol(args[1], &endptr, 10);
        if (*endptr != '\0' || job_num <= 0) {
            printf("No such job\n");
            return 1;
        }
        
        job_index = find_job_by_number((int)job_num);
        if (job_index == -1) {
            printf("No such job\n");
            return 1;
        }
    }
    
    printf("%s\n", jobs[job_index].command);
    
    // If stopped, send SIGCONT
    if (jobs[job_index].state == JOB_STOPPED) {
        kill(jobs[job_index].pid, SIGCONT);
    }
    
    foreground_pid = jobs[job_index].pid;
    
    int status;
    pid_t pid = jobs[job_index].pid;
    
    char* cmd_copy = strdup(jobs[job_index].command);
    remove_job(job_index);
    
    waitpid(pid, &status, WUNTRACED);
    
    if (WIFSTOPPED(status)) {
        add_stopped_process(pid, cmd_copy);
    }
    
    free(cmd_copy);
    foreground_pid = 0;
    
    return 1;
}

// E.4: bg command
int cmd_bg(char** args) {
    int job_index = -1;
    
    if (args[1] == NULL) {
        // Use most recent stopped job
        for (int i = job_count - 1; i >= 0; i--) {
            if (jobs[i].state == JOB_STOPPED) {
                job_index = i;
                break;
            }
        }
        if (job_index == -1) {
            printf("No such job\n");
            return 1;
        }
    } else {
        // Parse job number
        char* endptr;
        long job_num = strtol(args[1], &endptr, 10);
        if (*endptr != '\0' || job_num <= 0) {
            printf("No such job\n");
            return 1;
        }
        
        job_index = find_job_by_number((int)job_num);
        if (job_index == -1) {
            printf("No such job\n");
            return 1;
        }
    }
    
    if (jobs[job_index].state == JOB_RUNNING) {
        printf("Job already running\n");
        return 1;
    }
    
    kill(jobs[job_index].pid, SIGCONT);
    jobs[job_index].state = JOB_RUNNING;
    
    printf("[%d] %s &\n", jobs[job_index].job_number, jobs[job_index].command);
    
    return 1;
}
