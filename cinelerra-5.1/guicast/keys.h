
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

#ifndef KEYS_H
#define KEYS_H

#define XK_MeunKB 0x1008ff65

// key bindings
#define UP                  256
#define DOWN                257
#define LEFT                258
#define RIGHT               259
#define PGUP                260
#define PGDN                261
#define BACKSPACE           262
#define ESC                 263
#define TAB                 264
// Number pad keys
#define KPPLUS              265
#define KPMINUS             266
#define KPSTAR              267
#define KPSLASH             268
#define KP1                 269
#define KP2                 270
#define KP3                 271
#define KP4                 272
#define KP5                 273
#define KP6                 274
#define KP7                 275
#define KP8                 276
#define KP9                 277
#define KPINS               278
#define KPDEL               279
#define KPENTER             280
#define END                 281
#define HOME                282
#define LEFTTAB             283
#define DELETE              284

// ati_remote key bindings
//   requires some substitions in kernel module
//   due to deficiencies in X evdev et al.  examples:
//     need to know which dev created the event (duh)
//     keycodes > 255 are dropped
//     keycoodes are offset by 8 before xkb mapping
//     a bunch of keycodes are intercepted by windowmanger
// KEY_OK          = KEY_SPACE
// KEY_CHANNELUP   = KEY_SCROLLUP
// KEY_CHANNELDOWN = KEY_SCROLLDOWN
// KEY_PLAY        = KEY_FORWARD
// KEY_REWIND      = KEY_BACK
// KEY_FORWARD     = KEY_AGAIN
// - ati_remote.c, mapped in rc-ati-x10.c
#if 0
        { 0x18, KEY_KPENTER },          /* "check" */
        { 0x16, KEY_MENU },             /* "menu" */
        { 0x02, KEY_POWER },            /* Power */
        { 0x03, KEY_PROG1 },            /* TV */        /* was KEY_TV */
        { 0x04, KEY_PROG2 },            /* DVD */       /* was KEY_DVD */
        { 0x05, KEY_WWW },              /* WEB */
        { 0x06, KEY_PROG3 },            /* "book" */    /* was KEY_BOOKMARKS */
        { 0x07, KEY_PROG4 },            /* "hand" */    /* was KEY_EDIT */
        { 0x1c, KEY_REPLY },            /* "timer" */   /* was KEY_COFFEE */
        { 0x20, KEY_FRONT },            /* "max" */
        { 0x1d, KEY_LEFT },             /* left */
        { 0x1f, KEY_RIGHT },            /* right */
        { 0x22, KEY_DOWN },             /* down */
        { 0x1a, KEY_UP },               /* up */
        { 0x1e, KEY_SPACE },            /* "OK" */      /* was KEY_OK */
        { 0x09, KEY_VOLUMEDOWN },       /* VOL + */
        { 0x08, KEY_VOLUMEUP },         /* VOL - */
        { 0x0a, KEY_MUTE },             /* MUTE  */
        { 0x0b, KEY_SCROLLUP },         /* CH + */      /* was KEY_CHANNELUP */
        { 0x0c, KEY_SCROLLDOWN },       /* CH - */      /* was KEY_CHANNELDOWN */
        { 0x27, KEY_RECORD },           /* ( o) red */
        { 0x25, KEY_FORWARD },          /* ( >) */      /* was KEY_PLAY */
        { 0x24, KEY_BACK },             /* (<<) */      /* was KEY_REWIND */
        { 0x26, KEY_AGAIN },            /* (>>) */      /* was KEY_FORWARD */
        { 0x28, KEY_STOP },             /* ([]) */
        { 0x29, KEY_PAUSE },            /* ('') */
#endif
#define KPREV               284
#define KPMENU              285
#define KPTV                286
#define KPDVD               287
#define KPBOOK              288
#define KPHAND              289
#define KPTMR               290
#define KPMAXW              291
#define KPCHUP              292
#define KPCHDN              293
#define KPBACK              294
#define KPPLAY              295
#define KPFWRD              296
#define KPRECD              297
#define KPSTOP              298
#define KPAUSE              299
// function keys
#define KEY_F1              301
#define KEY_F2              302
#define KEY_F3              303
#define KEY_F4              304
#define KEY_F5              305
#define KEY_F6              306
#define KEY_F7              307
#define KEY_F8              308
#define KEY_F9              309
#define KEY_F10             310
#define KEY_F11             311
#define KEY_F12             312

#define RETURN              13
#define NEWLINE             13

#endif
