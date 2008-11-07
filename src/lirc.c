#include <lirc/lirc_client.h>
#include "lirc.h"

static struct lirc_config *lc = NULL;
static int lirc_socket = 0;

int ncmpc_lirc_open()
{
	if ((lirc_socket = lirc_init("ncmpc", 0)) == -1)
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

command_t ncmpc_lirc_get_command()
{
	char *code = NULL, *cmd = NULL;

	if (lirc_nextcode(&code) != 0)
		return CMD_NONE;

	if (lirc_code2char(lc, code, &cmd) != 0)
		return CMD_NONE;

	if (!cmd)
		return CMD_NONE;

	return get_key_command_from_name(cmd);
}
