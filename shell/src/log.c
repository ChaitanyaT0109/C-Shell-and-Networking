#include "shell.h"
#include "commands.h"
#include "parser.h"
#include "executor.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Log functionality
static char log_entries[MAX_LOG_SIZE][MAX_INPUT_SIZE];
static int log_count = 0;
static int log_start = 0; // For circular buffer

void add_to_log(const char* command) {
    // Don't log if command contains "log" as the main command
    char* cmd_copy = strdup(command);
    char* first_token = strtok(cmd_copy, " \t\n\r");
    if (first_token && strcmp(first_token, "log") == 0) {
        free(cmd_copy);
        return;
    }
    free(cmd_copy);
    
    // Don't add if identical to previous command
    if (log_count > 0) {
        int prev_index = (log_start + log_count - 1) % MAX_LOG_SIZE;
        if (strcmp(log_entries[prev_index], command) == 0) {
            return;
        }
    }
    
    if (log_count < MAX_LOG_SIZE) {
        strcpy(log_entries[log_count], command);
        log_count++;
    } else {
        // Circular buffer: overwrite oldest entry
        strcpy(log_entries[log_start], command);
        log_start = (log_start + 1) % MAX_LOG_SIZE;
    }
}

void save_log() {
    FILE* file = fopen(log_file_path, "w");
    if (file == NULL) {
        return;
    }
    
    for (int i = 0; i < log_count; i++) {
        int index = (log_start + i) % MAX_LOG_SIZE;
        fprintf(file, "%s\n", log_entries[index]);
    }
    
    fclose(file);
}

void load_log() {
    FILE* file = fopen(log_file_path, "r");
    if (file == NULL) {
        return;
    }
    
    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line), file) != NULL && log_count < MAX_LOG_SIZE) {
        line[strcspn(line, "\n")] = 0;
        strcpy(log_entries[log_count], line);
        log_count++;
    }
    
    fclose(file);
}

int cmd_log(char** args, int argc) {
    if (argc == 1) {
        // No arguments: display log
        for (int i = 0; i < log_count; i++) {
            int index = (log_start + i) % MAX_LOG_SIZE;
            printf("%s\n", log_entries[index]);
        }
    } else if (argc == 2 && strcmp(args[1], "purge") == 0) {
        // Purge log
        log_count = 0;
        log_start = 0;

// ############## LLM Generated Code Begins ##############
    } else if (argc == 3 && strcmp(args[1], "execute") == 0) {
        // Execute command at index
        char* endptr;
        long index = strtol(args[2], &endptr, 10);
        
        if (*endptr != '\0' || index <= 0 || index > log_count) {
            printf("log: Invalid Syntax!\n");
            return 1;
        }
// ############## LLM Generated Code Ends ##############
        
        int actual_index = (log_start + log_count - index) % MAX_LOG_SIZE;
        
        char* command_to_run = strdup(log_entries[actual_index]);
        
        char** tokens = tokenize(command_to_run);
        int result = execute_command(tokens);
        
        for (int i = 0; tokens[i] != NULL; i++) {
            free(tokens[i]);
        }
        free(tokens);
        free(command_to_run);
        
        return result;

        
    } else {
        printf("log: Invalid Syntax!\n");
    }
    
    return 1;
}

char* log_get_command_by_newest_index(int index) {
    if (index <= 0 || index > log_count) return NULL;
    int actual_index = (log_start + log_count - index) % MAX_LOG_SIZE;
    return strdup(log_entries[actual_index]);
}

int parse_log_execute_argv(char** argv) {
    if (!argv || !argv[0]) return 0;
    if (strcmp(argv[0], "log") != 0) return 0;
    if (!argv[1] || strcmp(argv[1], "execute") != 0) return 0;
    if (!argv[2]) return 0;
    char* endptr = NULL;
    long idx = strtol(argv[2], &endptr, 10);
    if (*argv[2] == '\0' || (endptr && *endptr != '\0')) return 0;
    if (idx <= 0 || idx > log_count) return 0;
    return (int)idx;
}