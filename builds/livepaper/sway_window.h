#ifndef SWAY_WINDOW_H
#define SWAY_WINDOW_H

#include <stdbool.h>

bool sway_window_is_session(void);
bool sway_window_prepare_livepaper_position(int x, int y);
bool sway_window_move_livepaper(int x, int y);
void sway_window_focus_livepaper(void);

#endif
