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

#include "imsss.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <errno.h>

#define BUFSIZE 1024

#define IMSSS_SOCKET                "/tmp/.imsss"

#define IMSSS_SUPPORT 0

static gboolean sigpipe_on = FALSE;

static void
handle_sigpipe() {
  sigpipe_on = TRUE;
}

#if IMSSS_SUPPORT
gboolean
imsss_set_status(gchar* str) {
  struct sockaddr_un    addr;
  int    fd;
  int    len;
  char   buf[BUFSIZE];
  int    ret;

  sigpipe_on = FALSE;

  signal(SIGPIPE, handle_sigpipe);

  if( str == NULL || *str == '\0' )
    return FALSE;

  if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
//    perror("socket");
    return FALSE;
  }

  bzero((char *)&addr, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, IMSSS_SOCKET);

  if (connect(fd, (struct sockaddr *)&addr,
    sizeof(addr.sun_family) + strlen(IMSSS_SOCKET)) < 0) {
//    perror("connect");
    close(fd);
    return FALSE;
  }

  ret = write(fd, str, strlen(str));
  close(fd);

  if( ret == -1 )
    return FALSE;

  if( sigpipe_on )
    return FALSE;

//g_print("TRUE %d, %d\n", ret, sigpipe_on);
  return TRUE;
}
#else
gboolean imsss_set_status(gchar* str) { return FALSE; }
#endif
