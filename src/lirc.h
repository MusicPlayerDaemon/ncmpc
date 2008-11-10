#ifndef LIRC_H
#define LIRC_H

#include "command.h"
#include <glib.h>

int ncmpc_lirc_open(void);
void ncmpc_lirc_close(void);

gboolean
lirc_event(GIOChannel *source, GIOCondition condition, gpointer data);

#endif
