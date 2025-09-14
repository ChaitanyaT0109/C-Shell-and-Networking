#ifndef PIPES_H
#define PIPES_H

#include "shell.h"
#include "redirection.h"

// Structure to hold pipeline information
typedef struct {
    char*** commands;  // Array of command arrays
    int num_commands;  // Number of commands in pipeline
    redirection_t input_redir;   // Input redirection for first command
    redirection_t output_redir;  // Output redirection for last command
} pipeline_t;

// Function declarations
int execute_pipeline(pipeline_t* pipeline);
void parse_pipeline(char** tokens, pipeline_t* pipeline);
void cleanup_pipeline(pipeline_t* pipeline);
int count_pipes(char** tokens);

#endif
