#include <signal.h>
#include <stdlib.h>
#include "signalhandling.h"

int continue_execution = 1;

static void signal_handler(int sig) {
    if (sig == SIGINT) {
        continue_execution = 0;
    }
}

struct sigaction sinact;
void init_signal() {
    sinact.sa_handler = signal_handler;
    sigemptyset(&sinact.sa_mask);
    sinact.sa_flags = 0;
    sigaction(SIGINT, &sinact, NULL);
}
