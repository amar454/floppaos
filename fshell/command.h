#ifndef COMMAND_H
#define COMMAND_H

// Global flag to indicate when a command is ready
#include "fshell.h"
extern int command_ready;
extern char command[MAX_COMMAND_LENGTH];

extern char current_time_string[32];
#endif // COMMAND_H
