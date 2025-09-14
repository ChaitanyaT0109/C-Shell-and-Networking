#ifndef LOG_H
#define LOG_H

// Log functions
void add_to_log(const char* command);
void save_log(void);
void load_log(void);
int cmd_log(char** args, int argc);

char* log_get_command_by_newest_index(int index);
int parse_log_execute_argv(char** argv);

#endif