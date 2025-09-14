#include "commands.h"
#include "background.h"
#include "shell.h"
#include "log.h"

// Built-in command names
char* builtin_str[] = {
    "hop",
    "reveal", 
    "log",
    "activities",
    "ping",
    "fg",
    "bg",
    "exit"
};

// Built-in command functions
int (*builtin_func[]) (char**) = {
    &cmd_hop_wrapper,
    &cmd_reveal_wrapper,
    &cmd_log_wrapper,
    &cmd_activities_wrapper,
    &cmd_ping_wrapper,
    &cmd_fg_wrapper,
    &cmd_bg_wrapper,
    &shell_exit
};

int num_builtins() {
    return sizeof(builtin_str) / sizeof(char*);
}

// Wrapper functions to convert between interfaces
int cmd_hop_wrapper(char** args) {
    int argc = 0;
    while (args[argc] != NULL) argc++;
    return cmd_hop(args, argc);
}

int cmd_reveal_wrapper(char** args) {
    int argc = 0;
    while (args[argc] != NULL) argc++;
    return cmd_reveal(args, argc);
}

int cmd_log_wrapper(char** args) {
    int argc = 0;
    while (args[argc] != NULL) argc++;
    return cmd_log(args, argc);
}

int cmd_activities_wrapper(char** args) {
    return cmd_activities(args);
}

int cmd_ping_wrapper(char** args) {
    return cmd_ping(args);
}

int cmd_fg_wrapper(char** args) {
    return cmd_fg(args);
}

int cmd_bg_wrapper(char** args) {
    return cmd_bg(args);
}

int shell_exit(char** args) {
    (void)args;
    return 0;
}

int cmd_hop(char** args, int argc) {
    if (argc == 1) {
        char current[MAX_PATH_SIZE];
        getcwd(current, MAX_PATH_SIZE);
        
        if (chdir(shell_home) != 0) {
            perror("hop");
        } else {
            // Update previous directory to where we came from
            strcpy(prev_dir, current);
        }
        return 1;
    }
    
    char current[MAX_PATH_SIZE];
    getcwd(current, MAX_PATH_SIZE);
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(args[i], "~") == 0) {
            if (chdir(shell_home) != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(prev_dir, current);
                getcwd(current, MAX_PATH_SIZE);
            }
        } else if (strcmp(args[i], ".") == 0) {
            // Do nothing
        } else if (strcmp(args[i], "..") == 0) {
            if (chdir("..") != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(prev_dir, current);
                getcwd(current, MAX_PATH_SIZE);
            }
        } else if (strcmp(args[i], "-") == 0) {
            if (strlen(prev_dir) == 0) {
                // No previous directory
            } else {
                char temp[MAX_PATH_SIZE];
                strcpy(temp, current);
                if (chdir(prev_dir) != 0) {
                    printf("No such directory!\n");
                } else {
                    strcpy(prev_dir, temp);
                    getcwd(current, MAX_PATH_SIZE);
                }
            }
        } else {
            // Regular path
            if (chdir(args[i]) != 0) {
                printf("No such directory!\n");
            } else {
                strcpy(prev_dir, current);
                getcwd(current, MAX_PATH_SIZE);
            }
        }
    }
    
    return 1;
}

int compare_strings(const void* a, const void* b) {
    return strcmp(*(char**)a, *(char**)b);
}

int cmd_reveal(char** args, int argc) {
    int flag_a = 0, flag_l = 0;
    char* target_dir = NULL;
    int arg_count = 0;
    
    // Parse flags and arguments
    for (int i = 1; i < argc; i++) {
        if (args[i][0] == '-' && strcmp(args[i], "-") != 0) {
            // Process flags (but not the special "-" directory)
            for (int j = 1; args[i][j] != '\0'; j++) {
                if (args[i][j] == 'a') {
                    flag_a = 1;
                } else if (args[i][j] == 'l') {
                    flag_l = 1;
                }
            }
        } else {
            if (arg_count > 0) {
                printf("reveal: Invalid Syntax!\n");
                return 1;
            }
            target_dir = args[i];
            arg_count++;
        }
    }
    
    // Determine target directory
    char path[MAX_PATH_SIZE];
    if (target_dir == NULL) {
        getcwd(path, MAX_PATH_SIZE);
    } else if (strcmp(target_dir, "~") == 0) {
        strcpy(path, shell_home);
    } else if (strcmp(target_dir, ".") == 0) {
        getcwd(path, MAX_PATH_SIZE);
    } else if (strcmp(target_dir, "..") == 0) {
        getcwd(path, MAX_PATH_SIZE);
        char* last_slash = strrchr(path, '/');
        if (last_slash && last_slash != path) {
            *last_slash = '\0';
        }
    } else if (strcmp(target_dir, "-") == 0) {
        if (strlen(prev_dir) == 0) {
            printf("No such directory!\n");
            return 1;
        }
        strcpy(path, prev_dir);
    } else {
        // Regular path
        if (target_dir[0] == '/') {
            strcpy(path, target_dir);
        } else {
            getcwd(path, MAX_PATH_SIZE);
            strcat(path, "/");
            strcat(path, target_dir);
        }
    }
    
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("No such directory!\n");
        return 1;
    }
    
// ############## LLM Generated Code Begins ##############    
    char** entries = malloc(1000 * sizeof(char*));
    int entry_count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (!flag_a && entry->d_name[0] == '.') {
            continue; // Skip hidden files unless -a flag is set
        }
        entries[entry_count] = strdup(entry->d_name);
        entry_count++;
    }
    closedir(dir);
// ############## LLM Generated Code Ends ################       
    
    // Sort entries lexicographically
    qsort(entries, entry_count, sizeof(char*), compare_strings);
    
    if (flag_l) {
        for (int i = 0; i < entry_count; i++) {
            printf("%s\n", entries[i]);
        }
    } else {
        for (int i = 0; i < entry_count; i++) {
            printf("%s", entries[i]);
            if (i < entry_count - 1) {
                printf(" ");
            }
        }
        if (entry_count > 0) {
            printf("\n");
        }
    }
    
    for (int i = 0; i < entry_count; i++) {
        free(entries[i]);
    }
    free(entries);
    
    return 1;
}
