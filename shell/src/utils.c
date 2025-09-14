#include "utils.h"

void get_shell_home() {
    char* cwd = getcwd(shell_home, MAX_PATH_SIZE);
    if (cwd == NULL) {
        // Fallback to HOME if getcwd fails
        char* home = getenv("HOME");
        if (home != NULL) {
            strcpy(shell_home, home);
        } else {
            strcpy(shell_home, "/");
        }
    }
}
