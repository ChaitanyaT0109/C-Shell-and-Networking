#include "shell.h"
#include "background.h"
#include "log.h"
#include "parser.h"
#include "executor.h"

// Global variables
char shell_home[MAX_PATH_SIZE];
char prev_dir[MAX_PATH_SIZE];
char log_file_path[MAX_PATH_SIZE];

int main() {
    init_shell();
    shell_loop();
    return 0;
}

void init_shell() {
    // Get and store shell home directory
    get_shell_home();
    
    // Initialize previous directory as empty
    prev_dir[0] = '\0';
    
    size_t home_len = strlen(shell_home);
    if (home_len + 11 < MAX_PATH_SIZE) {
        strcpy(log_file_path, shell_home);
        strcat(log_file_path, "/.shell_log");
    } else {
        strcpy(log_file_path, "/tmp/.shell_log");
    }
    
    // Load existing log
    load_log();
    
    // Setup signal handlers for Part E
    setup_signal_handlers();
}

void shell_loop() {
    char* line;
    
    while (1) {
        check_background_processes();
        
        display_prompt();
        line = read_input();
        
        // Skip empty input
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        if (strncmp(line, "; ", 2) == 0) {
            char* real_command = line + 2;
            
            add_to_log(real_command);
            save_log();
            
            int command_result = process_command(real_command);
            if (command_result == 0) {
                save_log();
                free(line);
                break;
            }
            
        } else {
            // This is for regular command processing.
            if (!validate_syntax(line)) {
                printf("Invalid Syntax!\n");
                add_to_log(line);
                save_log();
                free(line);
                continue;
            }
            
            int command_result = process_command(line);
            if (command_result == 0) {
                save_log(); // Save log before exiting
                free(line);
                break; // Exit shell
            }
            
            if (command_result == 1) {
                add_to_log(line);
                save_log();
            }
        }
        
        free(line);
    }
}

char** retokenize_and_execute(char* line) {
    char** tokens = tokenize(line);
    execute_command(tokens);
    return tokens;
}
