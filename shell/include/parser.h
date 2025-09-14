#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

char** tokenize(char* line);
void free_tokens(char** tokens);

#endif
