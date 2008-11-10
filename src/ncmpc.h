#ifndef NCMPC_H
#define NCMPC_H

#include "command.h"

void
sigstop(void);

void begin_input_event(void);
void end_input_event(void);
int do_input_event(command_t cmd);

#endif /* NCMPC_H */
