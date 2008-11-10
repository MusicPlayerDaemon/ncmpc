#include <lirc/lirc_client.h>
#include "lirc.h"
#include "ncmpc.h"

static struct lirc_config *lc = NULL;

int ncmpc_lirc_open()
{
	char prog[] = "ncmpc";
	int lirc_socket = 0;

	if ((lirc_socket = lirc_init(prog, 0)) == -1)
		return -1;

	if (lirc_readconfig(NULL, &lc, NULL)) {
		lirc_deinit();
		return -1;
	}

	return lirc_socket;
}

void ncmpc_lirc_close()
{
	if (lc)
		lirc_freeconfig(lc);
	lirc_deinit();
}

gboolean
lirc_event(mpd_unused GIOChannel *source,
	   mpd_unused GIOCondition condition, mpd_unused gpointer data)
{
	char *code, *txt;
	command_t cmd;

	begin_input_event();

	if (lirc_nextcode(&code) == 0) {
		while (lirc_code2char(lc, code, &txt) == 0 && txt != NULL) {
			cmd = get_key_command_from_name(txt);
			if (do_input_event(cmd) != 0)
				return FALSE;
		}
	}

	end_input_event();
	return TRUE;
}
