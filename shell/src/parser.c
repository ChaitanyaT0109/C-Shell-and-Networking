#include "shell.h"
#include "executor.h"

int validate_syntax(char* input) {
    if (input == NULL || strlen(input) == 0) {
        return 1; // Empty input is valid
    }
    
    char* trimmed = input;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    
    if (*trimmed == '|' || *trimmed == '&' || *trimmed == ';') {
        return 0;
    }
    
    if (strstr(input, "| ;") != NULL) {
        return 0;
    }
    
    size_t len = strlen(input);
    for (size_t i = 0; i < len - 1; i++) {
        if (input[i] == '|' && input[i+1] == ' ') {
            size_t j = i + 1;
            while (j < len && (input[j] == ' ' || input[j] == '\t')) j++;
            if (j < len && input[j] == ';') {
                return 0;
            }
        }
    }
    
    return 1;
}

char** tokenize(char* line) {
    int token_count = 0;
    char** tokens = malloc(MAX_NUM_TOKENS * sizeof(char*));
    
    int i = 0;
    int len = strlen(line);
    
    while (i < len && token_count < MAX_NUM_TOKENS - 1) {
        // Skip whitespace
        while (i < len && (line[i] == ' ' || line[i] == '\t')) {
            i++;
        }
        
        if (i >= len) break;
        
        if (line[i] == ';' || line[i] == '|' || line[i] == '&') {
            char* token = malloc(2);
            token[0] = line[i];
            token[1] = '\0';
            tokens[token_count] = token;
            token_count++;
            i++;
            continue;
        }
        
        int start = i;
// ############## LLM Generated Code Begins ##############
        int in_quotes = 0;
        char quote_char = '\0';
        
        while (i < len) {
            if (!in_quotes && (line[i] == ' ' || line[i] == '\t' || 
                              line[i] == ';' || line[i] == '|' || line[i] == '&')) {
                break;
            } else if (!in_quotes && (line[i] == '\'' || line[i] == '"')) {
                in_quotes = 1;
                quote_char = line[i];
                i++;
            } else if (in_quotes && line[i] == quote_char) {
                in_quotes = 0;
                quote_char = '\0';
                i++;
            } else {
                i++;
            }
        }
        
        int token_len = i - start;
        char* token = malloc(token_len + 1);
        strncpy(token, line + start, token_len);
        token[token_len] = '\0';
        
        if (token_len >= 2) {
            if ((token[0] == '\'' && token[token_len-1] == '\'') ||
                (token[0] == '"' && token[token_len-1] == '"')) {
                memmove(token, token + 1, token_len - 2);
                token[token_len - 2] = '\0';
            }
        }
        
        tokens[token_count] = token;
        token_count++;
    }
    
    tokens[token_count] = NULL;
    return tokens;
}
// ############## LLM Generated Code Ends ################

void free_tokens(char** tokens) {
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// Check if a command is a built-in command
int is_builtin(char* cmd) {
    char* builtin_commands[] = {"hop", "reveal", "log", "exit", NULL};
    
    for (int i = 0; builtin_commands[i] != NULL; i++) {
        if (strcmp(cmd, builtin_commands[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int looks_like_command(char* cmd) {
    // Built-in commands
    if (is_builtin(cmd)) {
        return 1;
    }
    
    char* common_commands[] = {
        "cat", "echo", "ls", "pwd", "grep", "sort", "head", "tail", 
        "wc", "tr", "cut", "awk", "sed", "find", "chmod", "cp", "mv", 
        "rm", "mkdir", "rmdir", "ps", "kill", "sleep", "date", "whoami",
        "which", "man", "less", "more", "vi", "vim", "nano", "emacs",
        NULL
    };
    
    for (int i = 0; common_commands[i] != NULL; i++) {
        if (strcmp(cmd, common_commands[i]) == 0) {
            return 1;
        }
    }
    
    char* common_english_words[] = {
        "Hi", "Hello", "This", "That", "The", "A", "An", "And", "Or", "But",
        "Is", "Are", "Was", "Were", "Be", "Been", "Have", "Has", "Had",
        "Do", "Does", "Did", "Will", "Would", "Could", "Should", "Can",
        "there", "here", "where", "when", "what", "why", "how", "who",
        "shell", "cool", "guys", "nice", "good", "bad", "great", "awesome",
        NULL
    };
    
    for (int i = 0; common_english_words[i] != NULL; i++) {
        if (strcasecmp(cmd, common_english_words[i]) == 0) {
            return 0;
        }
    }
    
    return 1;
}

int process_command(char* input) {
    // Tokenize input
    char** tokens = tokenize(input);
    
    if (tokens[0] == NULL) {
        // Empty command
        free_tokens(tokens);
        return 1;
    }
    
    int cmd_start = 0;
    int overall_result = 1;
    
    // Find commands separated by ';' and execute them sequentially
    for (int i = 0; tokens[i] != NULL || cmd_start < i; i++) {
        if (tokens[i] == NULL || strcmp(tokens[i], ";") == 0) {
            int cmd_length = i - cmd_start;
            
            if (cmd_length > 0) {
                char** current_cmd = malloc((cmd_length + 1) * sizeof(char*));
                for (int j = 0; j < cmd_length; j++) {
                    current_cmd[j] = strdup(tokens[cmd_start + j]);
                }
                current_cmd[cmd_length] = NULL;
                
                int result = execute_command(current_cmd);
                if (result == 0) {
                    overall_result = 0;
                    free_tokens(current_cmd);
                    break;
                }
                
                free_tokens(current_cmd);
            }
            
            cmd_start = i + 1;
            
            if (tokens[i] == NULL) {
                break;
            }
        }
    }
    
    // Free original tokens
    free_tokens(tokens);
    
    return overall_result;
}
