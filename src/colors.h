#ifndef COLORS_H
#define COLORS_H

#include <ncurses.h>

#define COLOR_TITLE           1
#define COLOR_TITLE_BOLD      2
#define COLOR_LINE            3
#define COLOR_LINE_BOLD       4
#define COLOR_LIST            5
#define COLOR_LIST_BOLD       6
#define COLOR_PROGRESSBAR     7
#define COLOR_STATUS          8
#define COLOR_STATUS_BOLD     9
#define COLOR_STATUS_TIME    10
#define COLOR_STATUS_ALERT   11

short colors_str2color(const char *str);

int colors_assign(const char *name, const char *value);
int colors_define(const char *name, short r, short g, short b);
int colors_start(void);
int colors_use(WINDOW *w, int id);


#endif /* COLORS_H */



