#ifndef SHELL_H
#define SHELL_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 4096
#define MAX_TOKEN_SIZE 256
#define MAX_NUM_TOKENS 256
#define MAX_PATH_SIZE 4096
#define MAX_LOG_SIZE 15

extern char shell_home[MAX_PATH_SIZE];
extern char prev_dir[MAX_PATH_SIZE];
extern char log_file_path[MAX_PATH_SIZE];

void init_shell();
void display_prompt();
char* read_input();
int validate_syntax(char* input);
int process_command(char* input);
void shell_loop();

// Built-in command functions
int cmd_hop(char** args, int argc);
int cmd_reveal(char** args, int argc);

// Utility functions
void get_shell_home();
char* get_current_dir_display();
char* get_username();
char* get_hostname();

// Signal handling functions
void setup_signal_handlers();
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);

#endif
