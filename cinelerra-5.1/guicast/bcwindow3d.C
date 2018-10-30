
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define GL_GLEXT_PROTOTYPES
#include "bcpixmap.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"
#include "language.h"

// OpenGL functions in BC_WindowBase

#ifdef HAVE_GL
int BC_WindowBase::glx_fb_configs(int *attrs, GLXFBConfig *&cfgs)
{
	int ncfgs = 0;
	cfgs = glXChooseFBConfig(get_display(), get_screen(), attrs, &ncfgs);
	if( !cfgs ) ncfgs = 0;
	else if( !ncfgs && cfgs ) { XFree(cfgs);  cfgs = 0; }
	return ncfgs;
}

// expects prefix of attrs to be:
//	GLX_CONFIG_CAVEAT,	<opt>,
//	GLX_DRAWABLE_TYPE,	<opt>,
//	GLX_DOUBLEBUFFER,	True,
int BC_WindowBase::glx_test_fb_configs(int *attrs, GLXFBConfig *&cfgs,
		const char *msg, int &msgs)
{
	int ncfgs = glx_fb_configs(attrs+2, cfgs);
	if( ncfgs ) return ncfgs;
	if( msgs < 1 ) { ++msgs; /* printf(_("%s: trying fallback 1\n"), msg); */ }
	ncfgs = glx_fb_configs(attrs+0, cfgs);
	if( ncfgs ) return ncfgs;
	if( msgs < 2 ) { ++msgs; /* printf(_("%s: trying single buffering\n"), msg); */ }
	attrs[5] = False;
	ncfgs = glx_fb_configs(attrs+0, cfgs);
	if( ncfgs ) return ncfgs;
	if( msgs < 3 ) { ++msgs; /* printf(_("%s: trying fallback 2\n"), msg); */ }
	ncfgs = glx_fb_configs(attrs+2, cfgs);
	if( ncfgs ) return ncfgs;
	if( msgs < 4 ) { ++msgs; /* printf(_("%s: trying attributes None\n"), msg); */ }
	ncfgs = glx_fb_configs(None, cfgs);
	if( ncfgs ) return ncfgs;
	disable_opengl();
	printf(_("%s: opengl initialization failed failed\n"), msg);
	return 0;
}

GLXFBConfig *BC_WindowBase::glx_window_fb_configs()
{
	static int msgs = 0;
	if( !glx_fbcfgs_window ) {
		int fb_attrs[] = {
			GLX_CONFIG_CAVEAT,	GLX_SLOW_CONFIG,
			GLX_DRAWABLE_TYPE,	GLX_WINDOW_BIT | GLX_PBUFFER_BIT | GLX_PIXMAP_BIT,
			GLX_DOUBLEBUFFER,	True,
			GLX_RENDER_TYPE,	GLX_RGBA_BIT,
			GLX_ACCUM_RED_SIZE,	1,
			GLX_ACCUM_GREEN_SIZE,	1,
			GLX_ACCUM_BLUE_SIZE,	1,
			GLX_ACCUM_ALPHA_SIZE,	1,
			GLX_RED_SIZE,		8,
			GLX_GREEN_SIZE,		8,
			GLX_BLUE_SIZE,		8,
			GLX_ALPHA_SIZE,		8,
			None
		};
		n_fbcfgs_window = glx_test_fb_configs(fb_attrs, glx_fbcfgs_window,
			"BC_WindowBase::glx_window_fb_configs", msgs);
	}
	return glx_fbcfgs_window;
}

Visual *BC_WindowBase::get_glx_visual(Display *display)
{
	Visual *visual = 0;
	XVisualInfo *vis_info = 0;
	GLXFBConfig *fb_cfgs = glx_window_fb_configs();
	if( fb_cfgs ) {
		for( int i=0; !vis_info && i<n_fbcfgs_window; ++i ) {
			if( vis_info ) { XFree(vis_info);  vis_info = 0; }
			glx_fb_config = fb_cfgs[i];
			vis_info = glXGetVisualFromFBConfig(display, glx_fb_config);
		}
	}
	if( vis_info ) {
		visual = vis_info->visual;
		XFree(vis_info);
	}
	else
		glx_fb_config = 0;
	return visual;
}

GLXFBConfig *BC_WindowBase::glx_pbuffer_fb_configs()
{
	static int msgs = 0;
	if( !glx_fbcfgs_pbuffer ) {
		int fb_attrs[] = {
			GLX_CONFIG_CAVEAT,	GLX_SLOW_CONFIG,
			GLX_DRAWABLE_TYPE,	GLX_WINDOW_BIT | GLX_PBUFFER_BIT | GLX_PIXMAP_BIT,
			GLX_DOUBLEBUFFER,	True, //False,
			GLX_RENDER_TYPE,	GLX_RGBA_BIT,
			GLX_ACCUM_RED_SIZE,	1,
			GLX_ACCUM_GREEN_SIZE,	1,
			GLX_ACCUM_BLUE_SIZE,	1,
			GLX_ACCUM_ALPHA_SIZE,	1,
			GLX_RED_SIZE,		8,
			GLX_GREEN_SIZE,		8,
			GLX_BLUE_SIZE,		8,
			GLX_ALPHA_SIZE,		8,
			None
		};
		n_fbcfgs_pbuffer = glx_test_fb_configs(fb_attrs, glx_fbcfgs_pbuffer,
			"BC_WindowBase::glx_pbuffer_fb_configs", msgs);
	}
	return glx_fbcfgs_pbuffer;
}

GLXFBConfig *BC_WindowBase::glx_pixmap_fb_configs()
{
	static int msgs = 0;
	if( !glx_fbcfgs_pixmap ) {
		static int fb_attrs[] = {
			GLX_CONFIG_CAVEAT,	GLX_SLOW_CONFIG,
			GLX_DRAWABLE_TYPE,	GLX_PIXMAP_BIT | GLX_PBUFFER_BIT,
			GLX_DOUBLEBUFFER,	True, //False,
			GLX_RENDER_TYPE,	GLX_RGBA_BIT,
			GLX_RED_SIZE,		8,
			GLX_GREEN_SIZE,		8,
			GLX_BLUE_SIZE,		8,
			GLX_ALPHA_SIZE,		8,
			None
		};
		n_fbcfgs_pixmap = glx_test_fb_configs(fb_attrs, glx_fbcfgs_pixmap,
			"BC_WindowBase::glx_pixmap_fb_configs", msgs);
	}
	return glx_fbcfgs_pixmap;
}

GLXContext BC_WindowBase::glx_get_context()
{
	if( !glx_win_context && top_level->glx_fb_config )
		glx_win_context = glXCreateNewContext(
			top_level->get_display(), top_level->glx_fb_config,
			GLX_RGBA_TYPE, 0, True);
	if( !glx_win_context )
		printf("BC_WindowBase::get_glx_context %d: failed\n", __LINE__);
	return glx_win_context;
}

void BC_WindowBase::sync_lock(const char *cp)
{
	get_synchronous()->sync_lock(cp);
}

void BC_WindowBase::sync_unlock()
{
	get_synchronous()->sync_unlock();
}

GLXWindow BC_WindowBase::glx_window()
{
	if( !glx_win ) {
		glx_win = glXCreateWindow(top_level->display, top_level->glx_fb_config, win, 0);
	}
	return glx_win;
}

bool BC_WindowBase::glx_make_current(GLXDrawable draw, GLXContext glx_ctxt)
{
	return glXMakeContextCurrent(get_display(), draw, draw, glx_ctxt);
}

bool BC_WindowBase::glx_make_current(GLXDrawable draw)
{
	return glx_make_current(draw, glx_win_context);
}

#endif

void BC_WindowBase::enable_opengl()
{
#ifdef HAVE_GL
	glx_win = glx_window();
	if( !glx_win ) {
		printf("BC_WindowBase::enable_opengl %d: no glx window\n", __LINE__);
		exit(1);
	}
	GLXContext glx_context = glx_get_context();
	if( !glx_context ) {
		printf("BC_WindowBase::enable_opengl %d: no glx context\n", __LINE__);
		exit(1);
	}
	top_level->sync_display();
	get_synchronous()->is_pbuffer = 0;
	get_synchronous()->current_window = this;
	glx_make_current(glx_win, glx_context);
#endif
}

void BC_WindowBase::disable_opengl()
{
#ifdef HAVE_GL
// 	unsigned long valuemask = CWEventMask;
// 	XSetWindowAttributes attributes;
// 	attributes.event_mask = DEFAULT_EVENT_MASKS;
// 	XChangeWindowAttributes(top_level->display, win, valuemask, &attributes);
#endif
}

void BC_WindowBase::flip_opengl()
{
#ifdef HAVE_GL
	glXSwapBuffers(top_level->display, glx_win);
	glFlush();
#endif
}

unsigned int BC_WindowBase::get_shader(char *source, int *got_it)
{
	return get_resources()->get_synchronous()->get_shader(source, got_it);
}

void BC_WindowBase::put_shader(unsigned int handle, char *source)
{
	get_resources()->get_synchronous()->put_shader(handle, source);
}

int BC_WindowBase::get_opengl_server_version()
{
#ifdef HAVE_GL
	int maj, min;
	if(glXQueryVersion(get_display(), &maj, &min))
		return 100 * maj + min;
#endif
	return 0;
}
