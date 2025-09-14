#include "pipes.h"
#include "commands.h"
#include "background.h"
#include "log.h"
#include "parser.h"
#include <signal.h>
#include <unistd.h>

int count_pipes(char** tokens) {
    int pipe_count = 0;
    for (int i = 0; tokens[i] != NULL; i++) {
        if (strcmp(tokens[i], "|") == 0) {
            pipe_count++;
        }
    }
    return pipe_count;
}

void parse_pipeline(char** tokens, pipeline_t* pipeline) {
    int token_count = 0;
    while (tokens[token_count] != NULL) {
        token_count++;
    }
    
    pipeline->num_commands = count_pipes(tokens) + 1;
    pipeline->commands = malloc(pipeline->num_commands * sizeof(char**));
    
    // Initialize redirections
    pipeline->input_redir.type = REDIR_NONE;
    pipeline->input_redir.filename = NULL;
    pipeline->output_redir.type = REDIR_NONE;
    pipeline->output_redir.filename = NULL;
    
    int cmd_idx = 0;
    int token_start = 0;
    
    for (int i = 0; i <= token_count; i++) {
        if (i == token_count || strcmp(tokens[i], "|") == 0) {
            // Extract command from token_start to i-1
            int cmd_length = i - token_start;
            char** cmd_tokens_temp = malloc((cmd_length + 1) * sizeof(char*));
            
            for (int j = 0; j < cmd_length; j++) {
                cmd_tokens_temp[j] = tokens[token_start + j];
            }
            cmd_tokens_temp[cmd_length] = NULL;
            
            redirection_t input_redir, output_redir;
            char** clean_cmd_tokens;
            parse_redirections(cmd_tokens_temp, &clean_cmd_tokens, &input_redir, &output_redir);
            
            int log_idx = parse_log_execute_argv(clean_cmd_tokens);
            if (log_idx > 0) {
                char* repl = log_get_command_by_newest_index(log_idx);
                if (repl) {
                    char** repl_tokens = tokenize(repl);

                    for (int t=0; clean_cmd_tokens[t]!=NULL; t++) free(clean_cmd_tokens[t]);
                    free(clean_cmd_tokens);
                    free(repl);
                    pipeline->commands[cmd_idx] = repl_tokens;
                } else {
                    pipeline->commands[cmd_idx] = clean_cmd_tokens;
                }
            } else {
                pipeline->commands[cmd_idx] = clean_cmd_tokens;
            }
            
            if (cmd_idx == 0 && input_redir.type != REDIR_NONE) {
                pipeline->input_redir = input_redir;
            } else {
                cleanup_redirection(&input_redir);
            }
            
            if (cmd_idx == pipeline->num_commands - 1 && output_redir.type != REDIR_NONE) {
                pipeline->output_redir = output_redir;
            } else {
                cleanup_redirection(&output_redir);
            }
            
            free(cmd_tokens_temp);
            cmd_idx++;
            token_start = i + 1;
        }
    }
}

int execute_pipeline(pipeline_t* pipeline) {
    if (pipeline->num_commands == 0) {
        return 1;
    }
    
    if (pipeline->num_commands == 1) {
        char** args = pipeline->commands[0];
        if (args[0] == NULL) {
            return 1;
        }
        
        // Check for background execution
        int background = 0;
        int arg_count = 0;
        while (args[arg_count] != NULL) {
            arg_count++;
        }
        
        if (arg_count > 0 && strcmp(args[arg_count - 1], "&") == 0) {
            background = 1;
            free(args[arg_count - 1]);
            args[arg_count - 1] = NULL;
        }
        
        for (int i = 0; i < num_builtins(); i++) {
            if (strcmp(args[0], builtin_str[i]) == 0) {
                if (!background) {
                    int stdin_backup = dup(STDIN_FILENO);
                    int stdout_backup = dup(STDOUT_FILENO);
                    
                    if (apply_redirections(&pipeline->input_redir, &pipeline->output_redir) == -1) {
                        dup2(stdin_backup, STDIN_FILENO);
                        dup2(stdout_backup, STDOUT_FILENO);
                        close(stdin_backup);
                        close(stdout_backup);
                        return 1;
                    }
                    
                    int result = (*builtin_func[i])(args);
                    
                    dup2(stdin_backup, STDIN_FILENO);
                    dup2(stdout_backup, STDOUT_FILENO);
                    close(stdin_backup);
                    close(stdout_backup);
                    
                    return result;
                } else {
                    printf("Background built-in commands not supported\n");
                    return 1;
                }
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process

            setpgid(0, 0);
            
            if (background && pipeline->input_redir.type == REDIR_NONE) {
                int null_fd = open("/dev/null", O_RDONLY);
                if (null_fd != -1) {
                    dup2(null_fd, STDIN_FILENO);
                    close(null_fd);
                }
            }
            
            if (apply_redirections(&pipeline->input_redir, &pipeline->output_redir) == -1) {
                exit(EXIT_FAILURE);
            }
            
            if (execvp(args[0], args) == -1) {
                printf("Command not found!\n");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("fork");
            return 1;
        } else {
            // Parent process
            if (background) {
                size_t cap = 256; size_t len = 0;
                char* cmd_str = malloc(cap);
                cmd_str[0] = '\0';
                for (int ai = 0; args[ai] != NULL; ai++) {
                    size_t aLen = strlen(args[ai]);
                    if (len + aLen + 2 >= cap) { cap *= 2; cmd_str = realloc(cmd_str, cap); }
                    if (ai > 0) { cmd_str[len++] = ' '; cmd_str[len] = '\0'; }
                    strcpy(cmd_str + len, args[ai]);
                    len += aLen;
                }
                if (len + 3 >= cap) { cap *= 2; cmd_str = realloc(cmd_str, cap); }
                strcpy(cmd_str + len, " &");
                add_background_process(pid, cmd_str);
                return 1;
            } else {
                foreground_pid = pid;
                
                setpgid(pid, pid);
                
                // Wait for the process to complete or stop
                int status;
                waitpid(pid, &status, WUNTRACED);
                
                if (WIFSTOPPED(status)) {
                    // (Ctrl-Z), add to background jobs
                    char* cmd_str = malloc(256);
                    strcpy(cmd_str, args[0]);
                    add_stopped_process(pid, cmd_str);
                    free(cmd_str);
                }
                
                foreground_pid = 0; 
                
                if (WIFSTOPPED(status)) {
                    return 1;
                }
                return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 1 : 2;
            }
        }
        
        return 1;
    }
    
    // Multiple commands - create pipeline
    int pipes[pipeline->num_commands - 1][2];
    pid_t pids[pipeline->num_commands];
    
    // Check for background execution in pipeline (check last command)
    int background = 0;
    char** last_cmd = pipeline->commands[pipeline->num_commands - 1];
    int last_cmd_arg_count = 0;
    while (last_cmd[last_cmd_arg_count] != NULL) {
        last_cmd_arg_count++;
    }
    
    if (last_cmd_arg_count > 0 && strcmp(last_cmd[last_cmd_arg_count - 1], "&") == 0) {
        background = 1;
        free(last_cmd[last_cmd_arg_count - 1]);
        last_cmd[last_cmd_arg_count - 1] = NULL;
    }
    
    // Create all pipes
    for (int i = 0; i < pipeline->num_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    
// ############## LLM Generated Code Begins ##############    
    // Create child processes
    for (int i = 0; i < pipeline->num_commands; i++) {
        pids[i] = fork();
        
        if (pids[i] == 0) {
            // Child process
            
            // Set up process group (first process becomes group leader)
            if (i == 0) {
                setpgid(0, 0);
            } else {
                setpgid(0, getpgid(pids[0]));
            }
            
            // For background processes, redirect stdin to /dev/null if no redirection
            if (background && i == 0 && pipeline->input_redir.type == REDIR_NONE) {
                int null_fd = open("/dev/null", O_RDONLY);
                if (null_fd != -1) {
                    dup2(null_fd, STDIN_FILENO);
                    close(null_fd);
                }
            }
            
            // Apply input redirection for first command
            if (i == 0 && pipeline->input_redir.type != REDIR_NONE) {
                if (setup_input_redirection(pipeline->input_redir.filename) == -1) {
                    exit(EXIT_FAILURE);
                }
            }
            // Connect input to previous pipe (except for first command)
            else if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Apply output redirection for last command
            if (i == pipeline->num_commands - 1 && pipeline->output_redir.type != REDIR_NONE) {
                if (setup_output_redirection(pipeline->output_redir.filename, pipeline->output_redir.type) == -1) {
                    exit(EXIT_FAILURE);
                }
            }
            // Connect output to next pipe (except for last command)
            else if (i < pipeline->num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe file descriptors in child
            for (int j = 0; j < pipeline->num_commands - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            for (int k = 0; k < num_builtins(); k++) {
                if (strcmp(pipeline->commands[i][0], builtin_str[k]) == 0) {
                    // Execute built-in command in child process for pipeline
                    int result = (*builtin_func[k])(pipeline->commands[i]);
                    exit(result == 0 ? EXIT_FAILURE : EXIT_SUCCESS);
                }
            }
            
            // Not a built-in, execute external command
            if (execvp(pipeline->commands[i][0], pipeline->commands[i]) == -1) {
                printf("Command not found!\n");
                exit(EXIT_FAILURE);
            }
        } else if (pids[i] < 0) {
            perror("fork");
            return 1;
        }
    }
//############## LLM Generated Code Ends ################

    // Close all pipe file descriptors in parent
    for (int i = 0; i < pipeline->num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Handle background vs foreground execution
    if (background) {
        size_t cap = 256; size_t len = 0;
        char* cmd_str = malloc(cap);
        cmd_str[0] = '\0';
        for (int ci = 0; ci < pipeline->num_commands; ci++) {
            if (ci > 0) {
                if (len + 3 >= cap) { cap *= 2; cmd_str = realloc(cmd_str, cap); }
                strcpy(cmd_str + len, " | ");
                len += 3;
            }
            for (int ai = 0; pipeline->commands[ci][ai] != NULL; ai++) {
                size_t aLen = strlen(pipeline->commands[ci][ai]);
                if (len + aLen + 2 >= cap) { cap *= 2; cmd_str = realloc(cmd_str, cap); }
                if (ai > 0) { cmd_str[len++] = ' '; cmd_str[len] = '\0'; }
                strcpy(cmd_str + len, pipeline->commands[ci][ai]);
                len += aLen;
            }
        }
        if (len + 3 >= cap) { cap *= 2; cmd_str = realloc(cmd_str, cap); }
        strcpy(cmd_str + len, " &");
        add_background_process(pids[pipeline->num_commands - 1], cmd_str);

        return 1;
    } else {
    for (int i = 0; i < pipeline->num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    foreground_pid = 0;
}    return 1;
}

void cleanup_pipeline(pipeline_t* pipeline) {
    if (pipeline->commands != NULL) {
        for (int i = 0; i < pipeline->num_commands; i++) {
            if (pipeline->commands[i] != NULL) {
                for (int j = 0; pipeline->commands[i][j] != NULL; j++) {
                    free(pipeline->commands[i][j]);
                }
                free(pipeline->commands[i]);
            }
        }
        free(pipeline->commands);
        pipeline->commands = NULL;
    }
    
    cleanup_redirection(&pipeline->input_redir);
    cleanup_redirection(&pipeline->output_redir);
    pipeline->num_commands = 0;
}
