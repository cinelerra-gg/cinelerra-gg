
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

#include "bcpbuffer.h"
#include "bcresources.h"
#include "bcsignals.h"
#include "bcsynchronous.h"
#include "bcwindowbase.h"




BC_PBuffer::BC_PBuffer(int w, int h)
{
	reset();
	this->w = w;
	this->h = h;
	create_pbuffer(w, h);
}

BC_PBuffer::~BC_PBuffer()
{
#ifdef HAVE_GL
	BC_WindowBase::get_synchronous()->release_pbuffer(window_id, pbuffer);
#endif
}


void BC_PBuffer::reset()
{
#ifdef HAVE_GL
	pbuffer = 0;
	window_id = -1;
#endif
}

#ifdef HAVE_GL

GLXPbuffer BC_PBuffer::get_pbuffer()
{
	return pbuffer;
}
#endif


void BC_PBuffer::create_pbuffer(int w, int h)
{
#ifdef HAVE_GL
	int ww = (w + 3) & ~3, hh = (h + 3) & ~3;
	BC_WindowBase *current_window = BC_WindowBase::get_synchronous()->current_window;
	window_id = current_window->get_id();

	pbuffer = BC_WindowBase::get_synchronous()->get_pbuffer(ww, hh, &glx_context);
	if( pbuffer ) return;

	int pb_attrs[] = {
		GLX_PBUFFER_WIDTH, ww,
		GLX_PBUFFER_HEIGHT, hh,
		GLX_LARGEST_PBUFFER, False,
		GLX_PRESERVED_CONTENTS, True,
		None
	};

	Display *dpy = current_window->get_display();
	GLXFBConfig *fb_cfgs = current_window->glx_pbuffer_fb_configs();
	int nfb_cfgs = current_window->n_fbcfgs_pbuffer;
	XVisualInfo *visinfo = 0;

	for( int i=0; !pbuffer && i<nfb_cfgs; ++i ) {
		if( visinfo ) { XFree(visinfo);  visinfo = 0; }
		BC_Resources::error = 0;
                visinfo = glXGetVisualFromFBConfig(dpy, fb_cfgs[i]);
		if( !visinfo || BC_Resources::error ) continue;
		pbuffer = glXCreatePbuffer(dpy, fb_cfgs[i], pb_attrs);
		if( BC_Resources::error ) pbuffer = 0;
	}

// printf("BC_PBuffer::create_pbuffer %d current_config=%d visinfo=%p error=%d pbuffer=%p\n",
// __LINE__, current_config, visinfo, BC_Resources::error, pbuffer);

	if( pbuffer ) {
		glx_context = glXCreateContext(dpy, visinfo, current_window->glx_win_context, 1);
		BC_WindowBase::get_synchronous()->put_pbuffer(ww, hh, pbuffer, glx_context);
	}
	else
		printf("BC_PBuffer::create_pbuffer: failed\n");

	if( visinfo ) XFree(visinfo);
#endif
}


int bcgl_enable_ret; // debug

void BC_PBuffer::enable_opengl()
{
#ifdef HAVE_GL
	BC_WindowBase *current_window = BC_WindowBase::get_synchronous()->current_window;
	bcgl_enable_ret = current_window->glx_make_current(pbuffer);
	BC_WindowBase::get_synchronous()->is_pbuffer = 1;
#endif
}

