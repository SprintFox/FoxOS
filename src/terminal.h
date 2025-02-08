#ifndef TERMINAL_H
#define TERMINAL_H

#include "stdint.h"

#define TERMINAL_BUFFER_SIZE 256
#define TERMINAL_PROMPT "FoxOS> "

void terminal_init(void);
void terminal_run(void);

#endif 