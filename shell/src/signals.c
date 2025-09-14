#include "shell.h"
#include "background.h"
#include <signal.h>
#include <sys/wait.h>

void setup_signal_handlers() {
    struct sigaction sa_int, sa_tstp, sa_chld;
    
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    //SIGTSTP handler
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);

    //SIGCHLD handler
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);
}

// E.3: Ctrl-C handler (SIGINT)
void sigint_handler(int sig) {
    (void)sig;
    
    if (foreground_pid > 0) {
        kill(-foreground_pid, SIGINT);
    } else {
        printf("\n");
        display_prompt();
        fflush(stdout);
    }
}

// E.3: Ctrl-Z handler (SIGTSTP)
void sigtstp_handler(int sig) {
    (void)sig;
    
    if (foreground_pid > 0) {
        kill(-foreground_pid, SIGTSTP);
    } else {
        printf("\n");
        fflush(stdout);
    }
}

// SIGCHLD handler for background process management
void sigchld_handler(int sig) {
    (void)sig;
    handle_background_processes();
}

void signal_handler(int sig) {
    if (sig == SIGCHLD) {
        handle_background_processes();
    }
}
