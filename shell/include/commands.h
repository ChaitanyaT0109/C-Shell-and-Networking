#ifndef COMMANDS_H
#define COMMANDS_H

#include "shell.h"
#include "log.h"

// Built-in command functions
int cmd_hop(char** args, int argc);
int cmd_reveal(char** args, int argc);

// Part E command functions
int cmd_activities(char** args);
int cmd_ping(char** args);
int cmd_fg(char** args);
int cmd_bg(char** args);

// Wrapper functions for executor interface
int cmd_hop_wrapper(char** args);
int cmd_reveal_wrapper(char** args);
int cmd_log_wrapper(char** args);
int cmd_activities_wrapper(char** args);
int cmd_ping_wrapper(char** args);
int cmd_fg_wrapper(char** args);
int cmd_bg_wrapper(char** args);
int shell_exit(char** args);

// Built-in command arrays
extern char* builtin_str[];
extern int (*builtin_func[]) (char**);
int num_builtins();

#endif
