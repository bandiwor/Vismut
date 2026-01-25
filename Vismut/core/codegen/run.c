#include "run.h"

#include <stdlib.h>
#include <stdio.h>

void Run(const char *filename, const char *exe_name) {
    char command_buffer[512];
    sprintf_s(command_buffer, sizeof(command_buffer),
              "gcc %s -O2 -m64 -march=native -s -static -o %s", filename, exe_name
    );
    system(command_buffer);

    sprintf_s((char *) command_buffer, sizeof(command_buffer),
              "%s", exe_name
    );
    system(command_buffer);
}
