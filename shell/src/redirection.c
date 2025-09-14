#include "redirection.h"

int setup_input_redirection(char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("No such file or directory\n");
        return -1;
    }
    
    if (dup2(fd, STDIN_FILENO) == -1) {
        close(fd);
        perror("dup2");
        return -1;
    }
    
    close(fd);
    return 0;
}

int setup_output_redirection(char* filename, redirection_type_t type) {
    int fd;
    
    if (type == REDIR_OUTPUT_TRUNC) {
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (type == REDIR_OUTPUT_APPEND) {
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    } else {
        return -1;
    }
    
    if (fd == -1) {
        printf("Unable to create file for writing\n");
        return -1;
    }
    
    if (dup2(fd, STDOUT_FILENO) == -1) {
        close(fd);
        perror("dup2");
        return -1;
    }
    
    close(fd);
    return 0;
}

void parse_redirections(char** tokens, char*** cmd_tokens, redirection_t* input_redir, redirection_t* output_redir) {
    int token_count = 0;
    while (tokens[token_count] != NULL) {
        token_count++;
    }
    
    // Initialize redirections
    input_redir->type = REDIR_NONE;
    input_redir->filename = NULL;
    output_redir->type = REDIR_NONE;
    output_redir->filename = NULL;
    
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 < token_count) {
                // Check if input file exists
                int fd = open(tokens[i + 1], O_RDONLY);
                if (fd == -1) {
                    // File doesn't exist, set error flag
                    input_redir->type = REDIR_ERROR;
                    if (input_redir->filename != NULL) {
                        free(input_redir->filename);
                    }
                    input_redir->filename = strdup(tokens[i + 1]);
                    
                    break;
                }
                close(fd);
                i++;
            }
        } else if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
            if (i + 1 < token_count) {
                int flags = (strcmp(tokens[i], ">>") == 0) ? 
                           (O_WRONLY | O_CREAT | O_APPEND) : 
                           (O_WRONLY | O_CREAT | O_TRUNC);
                int fd = open(tokens[i + 1], flags, 0644);
                if (fd == -1) {
                    output_redir->type = REDIR_ERROR;
                    if (output_redir->filename != NULL) {
                        free(output_redir->filename);
                    }
                    output_redir->filename = strdup(tokens[i + 1]);
                    
                    break;
                }
                close(fd);
                i++;
            }
        }
    }
    
    int cmd_token_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
            i++;
        } else {
            cmd_token_count++;
        }
    }
    
    *cmd_tokens = malloc((cmd_token_count + 1) * sizeof(char*));
    int cmd_idx = 0;
    
    if (input_redir->type == REDIR_ERROR || output_redir->type == REDIR_ERROR) {
        for (int i = 0; i < token_count; i++) {
            if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
                i++;
            } else {
                (*cmd_tokens)[cmd_idx] = strdup(tokens[i]);
                cmd_idx++;
            }
        }
        (*cmd_tokens)[cmd_idx] = NULL;
        return;
    }

// ############## LLM Generated Code Begins ##############    
    // Second pass: extract redirections
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 < token_count) {
                // Only keep the last input redirection
                if (input_redir->filename != NULL) {
                    free(input_redir->filename);
                }
                input_redir->type = REDIR_INPUT;
                input_redir->filename = strdup(tokens[i + 1]);
                i++; // Skip filename
            }
        } else if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 < token_count) {

                if (output_redir->filename != NULL) {
                    free(output_redir->filename);
                }
                output_redir->type = REDIR_OUTPUT_TRUNC;
                output_redir->filename = strdup(tokens[i + 1]);
                i++;
            }
        } else if (strcmp(tokens[i], ">>") == 0) {
            if (i + 1 < token_count) {
                if (output_redir->filename != NULL) {
                    free(output_redir->filename);
                }
                output_redir->type = REDIR_OUTPUT_APPEND;
                output_redir->filename = strdup(tokens[i + 1]);
                i++;
            }
// ############## LLM Generated Code Ends ################
        } else {
            (*cmd_tokens)[cmd_idx] = strdup(tokens[i]);
            cmd_idx++;
        }
    }
    (*cmd_tokens)[cmd_idx] = NULL;
}

void cleanup_redirection(redirection_t* redir) {
    if (redir->filename != NULL) {
        free(redir->filename);
        redir->filename = NULL;
    }
    redir->type = REDIR_NONE;
}

int apply_redirections(redirection_t* input_redir, redirection_t* output_redir) {
    if (input_redir->type == REDIR_ERROR) {
        printf("No such file or directory\n");
        return -1;
    }
    
    if (output_redir->type == REDIR_ERROR) {
        printf("Unable to create file for writing\n");
        return -1;
    }
    
    if (input_redir->type == REDIR_INPUT) {
        if (setup_input_redirection(input_redir->filename) == -1) {
            return -1;
        }
    }

    if (output_redir->type == REDIR_OUTPUT_TRUNC || output_redir->type == REDIR_OUTPUT_APPEND) {
        if (setup_output_redirection(output_redir->filename, output_redir->type) == -1) {
            return -1;
        }
    }
    
    return 0;
}
