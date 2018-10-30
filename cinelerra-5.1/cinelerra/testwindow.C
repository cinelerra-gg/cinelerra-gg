#include "testwindow.h"
// c++ -g -I../guicast testwindow.C ../guicast/x86_64/libguicast.a \
//  -DHAVE_GL -DHAVE_XFT -I/usr/include/freetype2 -lGL -lX11 -lXext \
//  -lXinerama -lXv -lpng  -lfontconfig -lfreetype -lXft -pthread

TestWindowGUI::
TestWindowGUI()
 : BC_Window("test", 100,100, 100,100, 100,100)
{
 	in_motion = 0;  last_x = last_y = -1;
	set_bg_color(BLACK);
	clear_box(0,0,get_w(),get_h());
	set_font(MEDIUMFONT);
	tx = get_text_ascent(MEDIUMFONT);
	ty = tx + get_text_height(MEDIUMFONT);
}

TestWindowGUI::
~TestWindowGUI()
{
}

int TestWindowGUI::button_press_event()
{
	if( !is_event_win() ) return 0;
	if( !cursor_inside() ) return 0;
	if( get_buttonpress() != 1 ) return 0;
	in_motion = 1;
	last_x = get_abs_cursor_x(0);
	last_y = get_abs_cursor_y(0);
	return 1;
}

int TestWindowGUI::button_release_event()
{
	last_x = last_y = -1;
	in_motion = 0;
	return 1;
}
int TestWindowGUI::cursor_motion_event()
{
	if( !in_motion ) return 1;
	char position[BCSTRLEN];
	int cx = get_abs_cursor_x(0), dx = cx-last_x;  last_x = cx;
	int cy = get_abs_cursor_y(0), dy = cy-last_y;  last_y = cy;
	reposition_window_relative(dx, dy);
	snprintf(position,sizeof(position),"%d,%d",get_x(),get_y());
	clear_box(0,0,get_w(),get_h());
	set_color(WHITE);
	draw_text(tx,ty, position);
	flash();
	return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int ac, char **av)
{
	BC_Signals signals;
	signals.initialize();
	TestWindow test_window;
	test_window.join();
	return 0;
}

