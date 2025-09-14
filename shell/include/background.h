#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "shell.h"

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED
} job_state_t;

typedef struct {
    pid_t pid;
    char* command;
    int job_number;
    job_state_t state;
} bg_process_t;

// Global variables for job control
extern bg_process_t* jobs;
extern int job_count;
extern int max_jobs;
extern pid_t foreground_pid;

void handle_background_processes();
void add_background_process(pid_t pid, char* command);
void add_stopped_process(pid_t pid, char* command);
int get_next_job_number();
void check_background_processes();
void remove_job(int index);
int find_job_by_pid(pid_t pid);
int find_job_by_number(int job_number);

// Part E commands
int cmd_activities(char** args);
int cmd_ping(char** args);
int cmd_fg(char** args);
int cmd_bg(char** args);

#endif
