#pragma once

extern unsigned short notification_bar[];

void notification_init(int full_width, int view_width, int show_banner, char *version_str);
void notification_update(int mode, float gamma, float ratio, int motion_check);
void notification_draw(unsigned short *dst);
