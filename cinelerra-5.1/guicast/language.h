
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

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include <libintl.h>
#include <string.h>

#define _(msgid) gettext(msgid)
#define gettext_noop(msgid) msgid
#define N_(msgid) gettext_noop(msgid)
#define _str(i)#i
#define str_(s)_str(s)

// for contextual use:
//  #define MSGQUAL qual_id
// C_: msgid decorated as: MSGQUAL#msg_id implicitly
// D_: msgid decorated as: qual_id#msg_id explicitly
// see po/xlat.sh for details
//
// qualifier from MSGQUAL
#define C_(msgid) msgqual(str_(MSGQUAL),msgid)
// qualifier from msgid
#define D_(msgid) msgtext(msgid)

static inline char *msgtext(const char *msgid)
{
  char *msgstr = gettext(msgid);
  if( msgstr == msgid ) {
    for( char *cp=msgstr; *cp!=0; ) if( *cp++ == '#' ) return gettext(cp);
    msgstr = (char*) msgid;
  }
  return msgstr;
}

static inline char *msgqual(const char *msgqual,const char *msgid)
{
  if( !msgqual || !*msgqual ) return gettext(msgid);
  char msg[strlen(msgid) + strlen(msgqual) + 2], *cp = msg;
  for( const char *bp=msgqual; *bp!=0; *cp++=*bp++ );
  *cp++ = '#';
  for( const char *bp=msgid; *bp!=0; *cp++=*bp++ );
  *cp = 0;
  if( (cp=gettext(msg)) == msg ) cp = gettext(msgid);
  return cp;
}

#endif
