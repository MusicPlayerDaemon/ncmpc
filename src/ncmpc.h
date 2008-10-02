#ifndef NCMPC_H
#define NCMPC_H

/* welcome message time [s] */
#define SCREEN_WELCOME_TIME 10

/* status message time [s] */
#define SCREEN_STATUS_MESSAGE_TIME 3

/* getch() timeout for non blocking read [ms] */
#define SCREEN_TIMEOUT 500

/* minumum window size */
#define SCREEN_MIN_COLS 14
#define SCREEN_MIN_ROWS  5

/* time between mpd updates [s] */
#define MPD_UPDATE_TIME 0.5

/* time before trying to reconnect [ms] */
#define MPD_RECONNECT_TIME  1500

void
sigstop(void);

#endif /* NCMPC_H */
