#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "shell.h"

// Redirection types
typedef enum {
    REDIR_NONE,
    REDIR_INPUT,
    REDIR_OUTPUT_TRUNC,
    REDIR_OUTPUT_APPEND,
    REDIR_ERROR
} redirection_type_t;

// To hold redirection information
typedef struct {
    redirection_type_t type;
    char* filename;
} redirection_t;

// Function declarations
int setup_input_redirection(char* filename);
int setup_output_redirection(char* filename, redirection_type_t type);
void parse_redirections(char** tokens, char*** cmd_tokens, redirection_t* input_redir, redirection_t* output_redir);
void cleanup_redirection(redirection_t* redir);
int apply_redirections(redirection_t* input_redir, redirection_t* output_redir);

#endif
