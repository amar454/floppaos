#ifndef FSHELL_H
#define FSHELL_H
#define MAX_COMMAND_LENGTH 128 // Define max command length
#define MAX_ARGUMENTS 10       // Define max number of arguments
#define IS_FS 0
#define IS_TMP_FS 1
void fshell_task(void *arg);
#endif // FSHELL_H
