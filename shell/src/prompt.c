#include "shell.h"

void display_prompt() {
    char* username = get_username();
    char* hostname = get_hostname();
    char* current_dir = get_current_dir_display();
    
    printf("<%s@%s:%s> ", username, hostname, current_dir);
    fflush(stdout);
    
    free(username);
    free(hostname);
    free(current_dir);
}

char* get_username() {
    char* username = getenv("USER");
    if (username == NULL) {
        struct passwd* pw = getpwuid(getuid());
        if (pw != NULL) {
            username = pw->pw_name;
        } else {
            username = "unknown";
        }
    }
    return strdup(username);
}

char* get_hostname() {
    struct utsname sys_info;
    if (uname(&sys_info) == 0) {
        return strdup(sys_info.nodename);
    }
    return strdup("unknown");
}

char* get_current_dir_display() {
    char* cwd = getcwd(NULL, 0);
    if (cwd == NULL) {
        return strdup("unknown");
    }
    
    // Check if current directory is under home directory
    if (strncmp(cwd, shell_home, strlen(shell_home)) == 0) {
        if (strcmp(cwd, shell_home) == 0) {
            free(cwd);
            return strdup("~");
        } else if (cwd[strlen(shell_home)] == '/') {
            char* relative_path = malloc(strlen(cwd) - strlen(shell_home) + 2);
            sprintf(relative_path, "~%s", cwd + strlen(shell_home));
            free(cwd);
            return relative_path;
        }
    }
    
    return cwd;
}
