/* 
 * Copyright (C) 2003 Yukihiro Nakai <nakai@gnome.gr.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Yukihiro Nakai <nakai@gnome.gr.jp>
 *
 */

#ifndef _IMSSS_H
#define _IMSSS_H

#include <glib.h>

#define IMSSS_STATUS_KANA           "STATUS KANA"
#define IMSSS_STATUS_KANJI          "STATUS KANJI"
#define IMSSS_STATUS_NONE           "STATUS NONE"

gboolean imsss_set_status(gchar* str);

#endif /* _IMSSS_H */
