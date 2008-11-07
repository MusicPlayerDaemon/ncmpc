#ifndef LIRC_H
#define LIRC_H

#include "command.h"

int ncmpc_lirc_open(void);
void ncmpc_lirc_close(void);
command_t ncmpc_lirc_get_command(void);

#endif
