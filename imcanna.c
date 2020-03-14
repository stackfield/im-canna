/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2002 Yusuke Tabata
 * Copyright (C) 2003-2004 Red Hat, Inc.
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
 * Authors: Owen Taylor <otaylor@redhat.com>
 *          Yusuke Tabata <tee@kuis.kyoto-u.ac.jp>
 *          Yukihiro Nakai <ynakai@redhat.com>
 *
 */

#include <stdio.h>

#include <string.h>
#include <canna/jrkanji.h>

#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>
#include <gtk/gtkimmodule.h>

#include "im-canna-intl.h"

#define IM_CONTEXT_TYPE_CANNA (type_canna)
#define IM_CONTEXT_CANNA(obj) (GTK_CHECK_CAST((obj), type_canna, IMContextCanna))
#define IM_CONTEXT_CANNA_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IM_CONTEXT_TYPE_CANNA, IMContextCannaClass))

#ifdef BUFSIZ
#undef BUFSIZ
#define BUFSIZ 1024
#endif

#include "furigana.h"
#include "imsss.h"

/* [ あ�] */
#define CANNA_MODESTR_NORMAL "\x5b\x20\xa4\xa2\x20\x5d"
/* [拡張] */
#define CANNA_MODESTR_EXTENDED "\x5b\xb3\xc8\xc4\xa5\x5d"

#define MASK_CONTROL	GDK_CONTROL_MASK
#define MASK_SHIFT	GDK_SHIFT_MASK
#define MASK_CTLSFT	(GDK_SHIFT_MASK | GDK_CONTROL_MASK)
#define MASK_NONE	0

static struct _gdk2canna_keytable {
  guint mask; /* Modifier key mask */
  guint gdk_keycode; /* Gdk key symbols */
  guint canna_keycode; /* Canna Key code or raw hex code */
} gdk2canna_keytable[] = {
  { MASK_CONTROL,  GDK_c, 0x03 },
  { MASK_CONTROL,  GDK_h, 0x08 },
  { MASK_CONTROL,  GDK_u, 0x15 },
  { MASK_CONTROL,  GDK_l, 0x0c },
  { MASK_CONTROL,  GDK_m, 0x0d },
  { MASK_CONTROL,  GDK_j, 0x0a }, /* Not supported yet */
  { MASK_CONTROL,  GDK_k, 0x0b },
  { MASK_CONTROL,  GDK_d, 0x04 },
  { MASK_CONTROL,  GDK_o, 0x0f },
  { MASK_CONTROL,  GDK_i, 0x09 },
  { MASK_NONE, GDK_Henkan, 0x0e },

  { MASK_CONTROL,  GDK_b, 0x02 }, /* Alias to GDK_Left */
  { MASK_NONE, GDK_Up, 0x10 }, /* Alias to GDK_Page_Up */
  { MASK_NONE, GDK_Left, 0x02 },
  { MASK_CONTROL,  GDK_f, 0x06 }, /* Alias to GDK_Right */
  { MASK_NONE, GDK_Down, 0x0e }, /* Alias to GDK_Page_Down */
  { MASK_NONE, GDK_Right, 0x06 },
  { MASK_CONTROL,  GDK_n, 0x0e }, /* Alias to GDK_Page_Down */
  { MASK_NONE, GDK_Page_Down, 0x0e },
  { MASK_CONTROL,  GDK_p, 0x10 }, /* Alias to GDK_Page_Up */
  { MASK_NONE, GDK_Page_Up, 0x10 },
  { MASK_NONE, GDK_Return, 0x0D },
  { MASK_NONE, GDK_BackSpace, 0x08 },
  { MASK_CONTROL,  GDK_a, CANNA_KEY_Home }, /* Alias to GDK_Home */
  { MASK_NONE, GDK_Home, CANNA_KEY_Home  },
  { MASK_CONTROL,  GDK_e, CANNA_KEY_End }, /* Alias to GDK_End */
  { MASK_NONE, GDK_End, CANNA_KEY_End  },
  { MASK_CONTROL,  GDK_g, 0x07 },
  { MASK_NONE,  GDK_Muhenkan, 0x07 },

  { MASK_SHIFT, GDK_Right, CANNA_KEY_Shift_Right },
  { MASK_SHIFT, GDK_Left, CANNA_KEY_Shift_Left },
  { MASK_SHIFT, GDK_Mode_switch, 0x10 },

  { MASK_NONE, 0, 0 },
};

typedef struct _IMContextCanna {
  GtkIMContext parent;
  gint kslength;
  guchar* workbuf;
  guchar* kakutei_buf;
  jrKanjiStatus ks;
  int cand_stat;
  GtkWidget* candwin;
  GtkWidget* candlabel;
  PangoLayout* layout;
  GdkWindow* client_window;
  gboolean ja_input_mode; /* JAPANESE input mode or not */
  gboolean focus_in_candwin_show; /* the candidate window should be up at focus in */
  gboolean function_mode; /* Word Registration or other function mode */

  GtkWidget* modewin;
  GtkWidget* modelabel;

  gint canna_context; /* Cast from pointer - FIXME */
  gint candwin_pos_x, candwin_pos_y;
} IMContextCanna;

typedef struct _IMContextCannaClass {
  GtkIMContextClass parent_class;
} IMContextCannaClass;

GType type_canna = 0;

int debug_loop = 0;

static guint snooper_id = 0;
static GtkIMContext* focused_context = NULL;

static void im_canna_class_init (GtkIMContextClass *class);
static void im_canna_init (GtkIMContext *im_context);
static void im_canna_finalize(GObject *obj);
static gboolean im_canna_filter_keypress(GtkIMContext *context,
		                GdkEventKey *key);
static void im_canna_get_preedit_string(GtkIMContext *, gchar **str,
					PangoAttrList **attrs,
					gint *cursor_pos);
static guchar* euc2utf8(const guchar* str);
static void im_canna_force_to_kana_mode(IMContextCanna* cn);
static void im_canna_focus_in(GtkIMContext* context);
static void im_canna_focus_out(GtkIMContext* context);
static void im_canna_show_candwin(IMContextCanna* cn, gchar* candstr);
static void im_canna_hide_candwin(IMContextCanna* cn);
static void im_canna_update_candwin(IMContextCanna* cn);
static void im_canna_set_client_window(GtkIMContext* context, GdkWindow *win);
static gboolean im_canna_is_modechangekey(GtkIMContext *context, GdkEventKey *key);
static void im_canna_update_modewin(IMContextCanna* cn);
static void im_canna_set_cursor_location (GtkIMContext *context,
					  GdkRectangle *area);
static void im_canna_move_window(IMContextCanna* cn, GtkWidget* widget);

static GSList* im_canna_get_furigana(IMContextCanna* cn);
static GSList* im_canna_get_furigana_slist (gchar *kakutei_text_euc,
					    gchar *muhenkan_text_euc);
static gint im_canna_get_utf8_len_from_euc (guchar *text_euc);
static gint im_canna_get_utf8_pos_from_euc_pos (guchar *text_euc,
						gint    pos);
static gboolean im_canna_is_euc_char(guchar* p);
static gboolean im_canna_is_euc_hiragana_char(guchar* p);
static gboolean im_canna_is_euc_hiragana(guchar* text);
static gboolean im_canna_is_euc_kanji(guchar* text);
static gint im_canna_get_pre_hiragana_len (guchar *text);
static gint im_canna_get_post_hiragana_len (guchar *text);
static void im_canna_reset(GtkIMContext* context);
static guint get_canna_keysym(guint keyval, guint state);
static gboolean return_false(GtkIMContext* context, GdkEventKey* key);
static void im_canna_kakutei(IMContextCanna* cn);
static void im_canna_kill(IMContextCanna* cn);

static guint focus_id = 0;

static gboolean
snooper_func(GtkWidget* widget, GdkEventKey* event, gpointer data) {
  if( focused_context )
    return im_canna_filter_keypress(focused_context, event);

  return FALSE;
}

static void
scroll_cb(GtkWidget* widget, GdkEventScroll* event, IMContextCanna* cn) {
  switch(event->direction) {
  case GDK_SCROLL_UP:
    jrKanjiString(cn->canna_context, 0x02, cn->kakutei_buf, BUFSIZ, &cn->ks);
    im_canna_update_candwin(cn);
    break;
  case GDK_SCROLL_DOWN:
    jrKanjiString(cn->canna_context, 0x06, cn->kakutei_buf, BUFSIZ, &cn->ks);
    im_canna_update_candwin(cn);
    break;
  default:
    break;
  }
}

/* Hash Table Functions */
void
strhash_dumpfunc(gpointer key, gpointer value, gpointer user_data) {
  printf("key: %s\n", (guchar*)key);
}

void
strhash_removefunc(gpointer key, gpointer value, gpointer user_data) {
  g_free(key);
}


gboolean
roma2kana_canna(GtkIMContext* context, gchar newinput) {
  gint nbytes;

  IMContextCanna *cn = IM_CONTEXT_CANNA(context);

  if( cn->kslength == 0 ) {
    bzero(cn->workbuf, sizeof(cn->workbuf));
    bzero(cn->kakutei_buf, sizeof(cn->kakutei_buf));
  }

  nbytes = jrKanjiString(cn->canna_context, newinput, cn->kakutei_buf, BUFSIZ, &cn->ks);

  if( cn->ks.length == -1 )
    return FALSE;

  cn->kslength = cn->ks.length;

  if( nbytes > 0 ) {
    gchar* euc = g_strndup(cn->kakutei_buf, nbytes);
    gchar* utf8 = euc2utf8(euc);
    g_signal_emit_by_name(cn, "commit", utf8);
    g_free(euc);
    g_free(utf8);
  }

  memset(cn->workbuf, 0, BUFSIZ);
  strncpy(cn->workbuf, cn->ks.echoStr, cn->kslength);
  g_signal_emit_by_name(cn, "preedit_changed");

  if( cn->ks.info & KanjiModeInfo ) {
    im_canna_update_modewin(cn);
  }
  im_canna_update_candwin(cn);

  return TRUE;
}

void
im_canna_register_type (GTypeModule *module)
{
  static const GTypeInfo object_info =
  {
    sizeof (IMContextCannaClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) im_canna_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (IMContextCanna),
    0,
    (GtkObjectInitFunc) im_canna_init,
  };

  type_canna = 
    g_type_module_register_type (module,
				 GTK_TYPE_IM_CONTEXT,
				 "IMContextCanna",
				 &object_info, 0);
}

static void
im_canna_class_init (GtkIMContextClass *class)
{
  GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS(class);
  GObjectClass *object_class = G_OBJECT_CLASS(class);

  im_context_class->set_client_window = im_canna_set_client_window;
/* Use snooper */
  im_context_class->filter_keypress = return_false;
/*
  im_context_class->filter_keypress = im_canna_filter_keypress;
*/
  im_context_class->get_preedit_string = im_canna_get_preedit_string;
  im_context_class->focus_in = im_canna_focus_in;
  im_context_class->focus_out = im_canna_focus_out;
  im_context_class->reset = im_canna_reset;
  im_context_class->set_cursor_location = im_canna_set_cursor_location;

  object_class->finalize = im_canna_finalize;
}

static void
im_canna_init (GtkIMContext *im_context)
{
  IMContextCanna *cn = IM_CONTEXT_CANNA(im_context);
  cn->canna_context = (int)cn;
  cn->cand_stat = 0;
  cn->kslength = 0;
  cn->workbuf = g_new0(guchar, BUFSIZ);
  cn->kakutei_buf = g_new0(guchar, BUFSIZ);
  cn->candwin = gtk_window_new(GTK_WINDOW_POPUP);

  gtk_widget_add_events(cn->candwin, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(cn->candwin, "scroll_event", G_CALLBACK(scroll_cb), cn);

  jrKanjiControl(cn->canna_context, KC_INITIALIZE, 0);

  cn->candlabel = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(cn->candwin), cn->candlabel);
  cn->layout  = gtk_widget_create_pango_layout(cn->candwin, "");
  cn->client_window = NULL;
  cn->focus_in_candwin_show = FALSE;
  cn->function_mode = FALSE;
  
  cn->modewin = gtk_window_new(GTK_WINDOW_POPUP);
  cn->modelabel = gtk_label_new("");
  gtk_container_add(GTK_CONTAINER(cn->modewin), cn->modelabel);

  snooper_id = gtk_key_snooper_install((GtkKeySnoopFunc)snooper_func, NULL);

  cn->candwin_pos_x = cn->candwin_pos_y = 0;

  im_canna_force_to_kana_mode(cn);

}

static void
im_canna_finalize(GObject *obj) {
  IMContextCanna* cn = IM_CONTEXT_CANNA(obj);

  jrKanjiControl(cn->canna_context, KC_FINALIZE, 0);

  if( snooper_id != 0 ) {
    gtk_key_snooper_remove(snooper_id);
    snooper_id = 0;
  }

  g_free(cn->workbuf);
  g_free(cn->kakutei_buf);
  gtk_widget_destroy(cn->candlabel);
  gtk_widget_destroy(cn->candwin);
  g_object_unref(cn->layout);
  gtk_widget_destroy(cn->modelabel);
  gtk_widget_destroy(cn->modewin);
}

static const GtkIMContextInfo im_canna_info = { 
  "Canna",		   /* ID */
  N_("Canna"),             /* Human readable name */
  "im-canna",		   /* Translation domain */
   IM_LOCALEDIR,	   /* Dir for bindtextdomain (not strictly needed for "gtk+") */
   "ja"			   /* Languages for which this module is the default */
};

static const GtkIMContextInfo *info_list[] = {
  &im_canna_info
};

void
im_module_init (GTypeModule *module)
{
  im_canna_register_type(module);
}

void 
im_module_exit (void)
{
}

void 
im_module_list (const GtkIMContextInfo ***contexts,
		int                      *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

GtkIMContext *
im_module_create (const gchar *context_id)
{
  if (strcmp (context_id, "Canna") == 0)
    return GTK_IM_CONTEXT (g_object_new (type_canna, NULL));
  else
    return NULL;
}

gboolean
im_canna_filter_keypress(GtkIMContext *context, GdkEventKey *key)
{
  IMContextCanna *cn = IM_CONTEXT_CANNA(context);
  guchar canna_code = 0;

  if( key->type == GDK_KEY_RELEASE ) {
    return FALSE;
  };

  //g_print("IM-CANNA: keyval=0x%04X\n", key->keyval);

  if( im_canna_is_modechangekey(context, key) ) {
    cn->function_mode = FALSE;
    cn->ja_input_mode = !cn->ja_input_mode;
    /* Editable widget should pass mnemonic if ja-input-mode is on */
    g_object_set_data(G_OBJECT(context), "immodule-needs-mnemonic",
		      (gpointer)cn->ja_input_mode);
    im_canna_update_modewin(cn);

    if( cn->ja_input_mode == FALSE ) {
      im_canna_force_to_kana_mode(cn);
      memset(cn->workbuf, 0, BUFSIZ);
      memset(cn->kakutei_buf, 0, BUFSIZ);
      g_signal_emit_by_name(cn, "preedit_changed");
    }
    return TRUE;
  }

  /* English mode */
  if( cn->ja_input_mode == FALSE ) {
    gunichar keyinput = gdk_keyval_to_unicode(key->keyval);
    gchar ubuf[7];

    if( keyinput == 0 )
      return FALSE; /* Gdk handles unknown key as 0x0000 */

    if( key->state & GDK_CONTROL_MASK )
      return FALSE;

    /* For real char keys */
    memset(ubuf, 0, 7);
    g_unichar_to_utf8(keyinput, ubuf);
    g_signal_emit_by_name(cn, "commit", ubuf);
    return TRUE;
  }

  /*** Japanese mode ***/
  /* Function mode */
  if( cn->function_mode ) {
    gtk_widget_hide_all(GTK_WIDGET(cn->modewin));
    canna_code = get_canna_keysym(key->keyval, key->state);

    if( canna_code != 0 ) {
      gint nbytes = jrKanjiString(cn->canna_context, canna_code, cn->kakutei_buf, BUFSIZ, &cn->ks);
      if( nbytes == 2 ) {
        gchar* euc = g_strndup(cn->kakutei_buf, nbytes);
        gchar* utf8 = euc2utf8(euc);
        g_signal_emit_by_name(cn, "commit", utf8);
        g_free(euc);
        g_free(utf8);
      }
      if( cn->ks.length != -1 )
        cn->kslength = cn->ks.length;
      if( strlen(cn->kakutei_buf) == 1 && cn->kakutei_buf[0] == canna_code ) {
        cn->kakutei_buf[0] = '\0';
      }
      g_signal_emit_by_name(cn, "preedit_changed");
      im_canna_update_candwin(cn);
      if( GTK_WIDGET_VISIBLE(cn->modewin)) {
        /* Currently, cn->function_mode switch depends on
         * cn->modewin visibleness, but if future im-canna always shows
         * cn->modewin, this code will be broken.
         */
        cn->function_mode = FALSE;
      }
      return TRUE;
    }
    return FALSE;
  }

  /* No preedit char yet */
  if( cn->kslength == 0 ) {
    gchar ubuf[7];
    gunichar keyinput;
    memset(ubuf, 0, 7);

    if( key->state & GDK_CONTROL_MASK ) {
      switch(key->keyval) {
      case GDK_a:
      case GDK_b:
      case GDK_f:
      case GDK_e:
      case GDK_n:
      case GDK_p:
      case GDK_o:
        return FALSE;
        break;
      default:
        break;
      }
    }

    switch(key->keyval) {
    case GDK_space:
      g_signal_emit_by_name(cn, "commit", " ");
      return TRUE;
      break;
    case GDK_Return:
    case GDK_BackSpace:
    case GDK_Left:
    case GDK_Up:
    case GDK_Right:
    case GDK_Down:
    case GDK_Page_Up:
    case GDK_Page_Down:
    case GDK_End:
    case GDK_Insert:
    case GDK_Delete:
    case GDK_Shift_L:
    case GDK_Shift_R:
      return FALSE; /* Functions key handling depends on afterward codes */
      break;
    case GDK_Home:
      cn->function_mode = TRUE;
      break;
    default:
      break;
    }
  }

  canna_code = 0;
  switch(key->keyval) {
  case GDK_Return:
    if( !GTK_WIDGET_VISIBLE(cn->candwin) ) {
/*
 * Disable Furigana support code because it doesn't make sense
 * when kakutei-if-end-of-bunsetsu is set as 't' in ~/.canna
 * 
 */
#if 0
      im_canna_get_furigana(cn);
#endif
    }
    break;
  /* All unused key in jp106 keyboard is listed up below */
  case GDK_Shift_L:
  case GDK_Shift_R:
  case GDK_F1:
  case GDK_F2:
  case GDK_F3:
  case GDK_F4:
  case GDK_F5:
  case GDK_F6:
  case GDK_F7:
  case GDK_F8:
  case GDK_F9:
  case GDK_F10:
  case GDK_F11:
  case GDK_F12:
  case GDK_Scroll_Lock:
  case GDK_Pause:
  case GDK_Num_Lock:
  case GDK_Eisu_toggle:
  case GDK_Control_L:
  case GDK_Control_R:
  case GDK_Alt_L:
  case GDK_Alt_R:
  case GDK_Hiragana_Katakana:
  case GDK_Delete:
  case GDK_Insert:
  case GDK_KP_Home:
  case GDK_KP_Divide:
  case GDK_KP_Multiply:
  case GDK_KP_Subtract:
  case GDK_KP_Add:
  case GDK_KP_Insert:
  case GDK_KP_Delete:
  case GDK_KP_Enter:
  case GDK_KP_Left:
  case GDK_KP_Up:
  case GDK_KP_Right:
  case GDK_KP_Down:
  case GDK_KP_Begin:
  case GDK_KP_Page_Up:
  case GDK_KP_Page_Down:
  case GDK_Escape:
  case 0x0000: /* Unknown(Unassigned) key */
    return FALSE;
  default:
    canna_code = get_canna_keysym(key->keyval, key->state);
    break;
  }

  /* Tab to commit and focus next editable widget.
   *  This code can be removed if Mozilla turns to call
   *  gtk_im_context_focus_out() when focus moved from widget
   *  to widget.
   *
   *  But this code doesn't have inconsistency with focus out
   *  code, so it might not need to be removed at that time.
   */
  if( key->keyval == GDK_Tab ) {
    /*
    gchar* utf8 = NULL;
    memset(cn->workbuf, 0, BUFSIZ);
    strncpy(cn->workbuf, cn->ks.echoStr, cn->kslength);
    utf8 = euc2utf8(cn->workbuf);
    g_signal_emit_by_name(cn, "commit", utf8);
    im_canna_force_to_kana_mode(cn);
    memset(cn->workbuf, 0, BUFSIZ);
    memset(cn->kakutei_buf, 0, BUFSIZ);
    g_signal_emit_by_name(cn, "preedit_changed");
    */
    im_canna_reset(context);
    im_canna_update_candwin(cn);
    /*    g_free(utf8); */
    return FALSE;
  }

  /*
   * if cursor-wrap is off in canna setting file, prevent preedit area to hide
   * when left/right is pushed and Cursor has first/last.
   */
  if (!GTK_WIDGET_VISIBLE(cn->candwin)) {
    /* GDK_LEFT || Ctrl-B */
    if( canna_code == 0x02 && cn->ks.revPos == 0)
      return TRUE;
    /* GDK_RIGHT || Ctrl-F */
    if(cn->ks.length == cn->ks.revPos + cn->ks.revLen)
      if(canna_code == 0x06)
	return TRUE;
  }

  if( canna_code != 0 ) {
    memset(cn->kakutei_buf, 0, BUFSIZ);
    jrKanjiString(cn->canna_context, canna_code, cn->kakutei_buf, BUFSIZ, &cn->ks);
    if( cn->ks.length != -1 )
      cn->kslength = cn->ks.length;
    if( strlen(cn->kakutei_buf) == 1 && cn->kakutei_buf[0] == canna_code ) {
      cn->kakutei_buf[0] = '\0';
    }

/* NAKAI */
    if( *cn->kakutei_buf != '\0' ) {
      gchar* utf8 = euc2utf8(cn->kakutei_buf);
      g_signal_emit_by_name(cn, "commit", utf8);
      memset(cn->kakutei_buf, 0, BUFSIZ);
      memset(cn->workbuf, 0, BUFSIZ);
      cn->kslength = 0;
      g_free(utf8);
    }

    g_signal_emit_by_name(cn, "preedit_changed");
    im_canna_update_candwin(cn);
    return TRUE;
  }

  /* Pass char to Canna, anyway */
  if( roma2kana_canna(context, key->keyval) )
    return TRUE;

  return FALSE;
}

/*
 * index_mb2utf8:
 *     return utf8 index from multibyte string and multibyte index
 */
static int
index_mb2utf8(gchar* mbstr, int revPos) {
  gchar* u8str;
  gchar* mbbuf;
  gint ret = 0;

  if( mbstr == NULL || *mbstr == '\0' ) {
    return 0;
  }

  if( revPos <= 0 || strlen(mbstr) < revPos ) {
    return 0;
  }

  mbbuf = g_strndup(mbstr, revPos);
  u8str = euc2utf8 (mbbuf);
  g_free(mbbuf);

  ret = strlen(u8str);
  g_free(u8str);

  return ret;
}

void
im_canna_get_preedit_string(GtkIMContext *ic, gchar **str,
			    PangoAttrList **attrs, gint *cursor_pos)
{
  IMContextCanna *cn = IM_CONTEXT_CANNA(ic);
  gchar* eucstr = NULL;

  if (attrs) {
    *attrs = pango_attr_list_new();
  }
  if (cursor_pos) {
    *cursor_pos = 0;
  }

  if( cn->kslength == 0 ) {
    *str = g_strdup("");
    return;
  }

  if( cn->ks.echoStr == NULL || *cn->ks.echoStr == '\0' ) {
    *str = g_strdup("");
    return;
  }

  eucstr = g_strndup(cn->ks.echoStr, cn->kslength);

  *str = euc2utf8(eucstr);
  g_free(eucstr);

  if( *str == NULL || strlen(*str) == 0 ) return;

  if (attrs) {
    PangoAttribute *attr;
    attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
    attr->start_index = 0;
    attr->end_index = strlen(*str);
    pango_attr_list_insert(*attrs, attr);

    attr = pango_attr_background_new(0, 0, 0);
    attr->start_index = index_mb2utf8(cn->ks.echoStr, cn->ks.revPos);
    attr->end_index = index_mb2utf8(cn->ks.echoStr,
				    cn->ks.revPos+cn->ks.revLen);
    pango_attr_list_insert(*attrs, attr);

    attr = pango_attr_foreground_new(0xffff, 0xffff, 0xffff);
    attr->start_index = index_mb2utf8(cn->ks.echoStr, cn->ks.revPos);
    attr->end_index = index_mb2utf8(cn->ks.echoStr,
				    cn->ks.revPos+cn->ks.revLen);
    pango_attr_list_insert(*attrs, attr);
  }

  if (cursor_pos != NULL) {
    int charpos = 0 , eucpos = 0;

    while (cn->ks.revPos > eucpos) {
      unsigned char c = *(cn->ks.echoStr + eucpos);

      if (c < 0x80)
	eucpos += 1; // EUC G0 (ASCII) == GL
      else if (c == 0x8E)
	eucpos += 2; // EUC G2 (Half Width Kana) == SS2
      else if (c == 0x8F)
	eucpos += 3; // EUC G3 (JIS 3-4 level Kanji) == SS3
      else
	eucpos += 2; // EUC G1 (JIS 1-2 level Kanji) == GR

      charpos++;
    }

    *cursor_pos = charpos;
  }

}

static guchar *
euc2utf8 (const guchar *str)
{
  GError *error = NULL;
  gchar *result = NULL;
  gsize bytes_read = 0, bytes_written = 0;

  result = g_convert (str, -1,
		      "UTF-8", "EUC-JP",
		      &bytes_read, &bytes_written, &error);
  if (error) {
    /* Canna outputs ideograph codes of where EUC-JP is not assigned. */
    gchar* eucprefix = (bytes_read == 0) ? g_strdup("") : g_strndup(str, bytes_read);
    gchar* prefix = euc2utf8(eucprefix);
    /* 2 bytes skip */
    gchar* converted = euc2utf8(str + bytes_read + 2);
    if( result )
      g_free(result);
    /* Replace to a full-width space */
    result = g_strconcat(prefix, "\xe3\x80\x80", converted, NULL);
    g_free(prefix);
    g_free(eucprefix);
    g_free(converted);
  }

#if 0
  if (error)
    {
      g_warning ("Error converting text from IM to UTF-8: %s\n", error->message);
      g_print("Error: bytes_read: %d\n", bytes_read);
      g_print("%02X %02X\n", str[bytes_read], str[bytes_read+1]);
      g_print("Error: bytes_written: %d\n", bytes_written);
      g_error_free (error);
    }
#endif

  return result;
}

/* im_canna_force_to_kana_mode() just change mode to kana.
 */
static void
im_canna_force_to_kana_mode(IMContextCanna* cn) {
  jrKanjiStatusWithValue ksv;

  cn->kslength = 0;

  ksv.ks = &(cn->ks);
  ksv.val = CANNA_MODE_HenkanMode;
  ksv.buffer = cn->workbuf;
  ksv.bytes_buffer = BUFSIZ;
  jrKanjiControl(cn->canna_context, KC_CHANGEMODE, (void*)&ksv);

  im_canna_hide_candwin(cn);
  im_canna_update_modewin(cn);
}

static void
im_canna_set_client_window(GtkIMContext* context, GdkWindow *win) {
  IMContextCanna* cn = IM_CONTEXT_CANNA(context);

  cn->client_window = win;
}

static gint
eucpos2utf8pos(guchar* euc, gint eucpos) {
  guchar* tmpeuc = NULL;
  gchar* utf8str = NULL;
  gint result;

  if ( eucpos <= 0  || euc == NULL || *euc == '\0' )
    return 0;

  tmpeuc = g_strndup(euc, eucpos);
  utf8str = euc2utf8(tmpeuc);

  result = utf8str ? strlen(utf8str) : 0 ;

  if( utf8str )
    g_free(utf8str);
  g_free(tmpeuc);

  return result;
}

static void
im_canna_show_candwin(IMContextCanna* cn, gchar* candstr) {
  gchar* labeltext = euc2utf8(candstr);
  PangoAttrList* attrlist = pango_attr_list_new();
  PangoAttribute* attr;

#if 0 /* No glyph for 0x3000 in FC2 -- Looks like GTK+2 bug ? */
  while( g_utf8_strchr(labeltext, -1, 0x3000) ) {
    gchar* tmpbuf = NULL;
    gchar* tmpbuf2 = NULL;
    gchar* cursor = g_utf8_strchr(labeltext, -1, 0x3000);

    tmpbuf = g_strndup(labeltext, cursor - labeltext);
    tmpbuf2 = g_strdup(g_utf8_offset_to_pointer(cursor, 1));
    g_free(labeltext);
    labeltext = g_strconcat(tmpbuf, "  ", tmpbuf2, NULL);
    g_free(tmpbuf);
    g_free(tmpbuf2);
  }
#endif

  im_canna_move_window(cn, cn->candwin);

  gtk_label_set_text(GTK_LABEL(cn->candlabel), labeltext);
  attr = pango_attr_background_new(0, 0, 0);
  attr->start_index = eucpos2utf8pos(candstr, cn->ks.gline.revPos);
  attr->end_index = eucpos2utf8pos(candstr, cn->ks.gline.revPos + cn->ks.gline.revLen);
  pango_attr_list_insert(attrlist, attr);

  attr = pango_attr_foreground_new(0xffff, 0xffff, 0xffff);
  attr->start_index = eucpos2utf8pos(candstr, cn->ks.gline.revPos);
  attr->end_index = eucpos2utf8pos(candstr, cn->ks.gline.revPos + cn->ks.gline.revLen);
  pango_attr_list_insert(attrlist, attr);

  gtk_label_set_attributes(GTK_LABEL(cn->candlabel), attrlist);

  gtk_window_resize (GTK_WINDOW(cn->candwin), 1, 1);

  gtk_widget_show_all(cn->candwin);

  g_free(labeltext);

  im_canna_update_modewin(cn);
}

static void im_canna_hide_candwin(IMContextCanna* cn) {
  if( cn->candwin )
    gtk_widget_hide_all(cn->candwin);

  im_canna_update_modewin(cn);
}

/* Mode change key combination (Shift+Space etc) or not? */
static gboolean
im_canna_is_modechangekey(GtkIMContext *context, GdkEventKey *key) {
  /* Kinput2 style - Shift + Space */
  if( key->state & GDK_SHIFT_MASK && key->keyval == GDK_space ) {
    return TRUE;
  /* Chinese/Korean style - Control + Space */
  } else if( key->state & GDK_CONTROL_MASK && key->keyval == GDK_space ) {
    return TRUE;
  /* Windows Style - Alt + Kanji */
  } else if( key->keyval == GDK_Kanji ) {
    return TRUE;
  /* Egg style - Control + '\' */
  } else if( key->state & GDK_CONTROL_MASK && key->keyval == GDK_backslash ) {
    return TRUE;
  }
  /* or should be customizable with dialog */

  return FALSE;
}

/*
 * Strict Furigana Discard function
 * Utf8 used internally
 */
GSList*
discard_strict_furigana(gchar* ptext, GSList* furigana_slist) {
  return furigana_slist;
}

static GSList*
im_canna_get_furigana(IMContextCanna* cn) {
  GSList* furigana_slist = NULL;

  gchar* finalstr = NULL; /* Final input string */
  gchar buffer[BUFSIZ]; /* local buffer */

  Furigana* f;
  guchar* reftext;
  guchar* candtext = NULL;
  guchar* recovertext = NULL;
  gchar* tmpbuf = NULL;

  gint   chunk_kakutei_text_euc_pos;
  gchar *chunk_kakutei_text_euc;
  gchar *chunk_muhenkan_text_euc;

  g_return_val_if_fail(cn->kslength > 0, NULL);

  chunk_kakutei_text_euc_pos = -1;
  chunk_kakutei_text_euc  = NULL;
  chunk_muhenkan_text_euc = NULL;


  finalstr = g_strndup(cn->ks.echoStr, cn->kslength);

  /* Discard Furigana for Hiragana */
  if ( im_canna_is_euc_hiragana(finalstr) ) {
    g_object_set_data(G_OBJECT(cn), "furigana", NULL);
    g_free(finalstr);
    return NULL;
  }

  memset(buffer, 0, BUFSIZ);
  strcpy(buffer, finalstr);

  debug_loop = 0;

  while(cn->ks.revPos != 0) {

    jrKanjiString(cn->canna_context, 0x06, buffer, BUFSIZ, &cn->ks); /* Ctrl+F */

    //    printf("pos: %d\n", cn->ks.revPos);
    //    printf("Len: %d\n", cn->ks.revLen);

    if( debug_loop++ > 100 ) {
      /* Happens when the candidate window */
      // printf("FURIGANA: while loop exceeded. (>100) Bug!!\n");
      g_object_set_data(G_OBJECT(cn), "furigana", NULL);
      return NULL;
    } 
  }

  g_assert(cn->ks.revLen > 0);

  do {
    recovertext = NULL;
    reftext = g_strndup(cn->ks.echoStr+cn->ks.revPos, cn->ks.revLen);
    // printf("reftext: %s\n", reftext);
    if( im_canna_is_euc_hiragana(reftext) ) {
      g_free(reftext);
      jrKanjiString(cn->canna_context, 0x06, buffer, BUFSIZ, &cn->ks); /* Ctrl+F */
      continue;
    }
    
    chunk_kakutei_text_euc_pos = cn->ks.revPos;
    chunk_kakutei_text_euc     = g_strdup (reftext);

    if( !im_canna_is_euc_hiragana(reftext) ) {
      GHashTable* loop_check_ht = g_hash_table_new(g_str_hash, g_str_equal);
      candtext = g_strdup(reftext);
      recovertext = g_strdup(""); /* Something free-able but not reftext */
      /* Loop until Hiragana for Furigana shows up */
	while( !im_canna_is_euc_hiragana(candtext) ) {
	  guint strcount = 0;

	  g_free(candtext);
          jrKanjiString(cn->canna_context, 0x0e, buffer, BUFSIZ, &cn->ks); /* Ctrl+N */
          candtext = g_strndup(cn->ks.echoStr+cn->ks.revPos, cn->ks.revLen);
	  
	  strcount = (guint)g_hash_table_lookup(loop_check_ht, candtext);

	  if( strcount >= 4 ) { /* specify threshold */
	    // printf("Looping, No Furigana: %s\n", reftext);
	    g_hash_table_foreach(loop_check_ht, strhash_dumpfunc, NULL);
	    g_free(candtext);
	    candtext = NULL;
	    break;
	  }
	  
	  g_hash_table_insert(loop_check_ht, g_strdup(candtext),
			      (gpointer)++strcount);
	}
	g_hash_table_foreach(loop_check_ht, strhash_removefunc, NULL);
	g_hash_table_destroy(loop_check_ht);
	// printf("candtext: %s\n", candtext ? candtext : (guchar*)("NO-FURIGANA"));

	chunk_muhenkan_text_euc = g_strdup (candtext);

	loop_check_ht = g_hash_table_new(g_str_hash, g_str_equal);
	/* Loop again to recover the final kakutei text */
	while( strcmp(recovertext, reftext) != 0 ) {
	  guint strcount;

	  g_free(recovertext);
	  jrKanjiString(cn->canna_context, 0x0e, buffer, BUFSIZ, &cn->ks); /* Ctrl+N */
	  recovertext = g_strndup(cn->ks.echoStr+cn->ks.revPos, cn->ks.revLen);

	  strcount = (guint)g_hash_table_lookup(loop_check_ht, recovertext);
	  if( strcount >= 4 ) { /* threshold */
	    /* Loop detected - Give up recovery */
	    jrKanjiString(cn->canna_context, 0x07, buffer, BUFSIZ, &cn->ks); /* Ctrl+G */
	    printf("Cannot recover: %s\n", reftext);
	    g_hash_table_foreach(loop_check_ht, strhash_dumpfunc, NULL);
	    if( candtext )
	      g_free(candtext);
	    candtext = NULL;
	    break;
	  }
	  g_hash_table_insert(loop_check_ht, g_strdup(recovertext),
			      (gpointer)++strcount);
	}
	g_hash_table_foreach(loop_check_ht, strhash_removefunc, NULL);
	g_hash_table_destroy(loop_check_ht);
 
	if( recovertext != NULL )
	  g_free(recovertext);
	if( reftext != NULL )
	  g_free(reftext);
	reftext = candtext;
    }
    if ((chunk_muhenkan_text_euc != NULL) &&
	(*chunk_muhenkan_text_euc != '\0'))
      {
	gint offset0;
	GSList *tmp_slist;
	GSList *tmp;
	Furigana *furigana;

	offset0 = im_canna_get_utf8_pos_from_euc_pos
	  		(finalstr, chunk_kakutei_text_euc_pos);

	tmp_slist = im_canna_get_furigana_slist (chunk_kakutei_text_euc,
						 chunk_muhenkan_text_euc);

	tmp = tmp_slist;
	while (tmp != NULL)
	  {
	    furigana = (Furigana *)tmp->data;
	    tmp = g_slist_next (tmp);

	    furigana->offset += offset0;
	  }

	furigana_slist = g_slist_concat (furigana_slist, tmp_slist);
      }

    if (chunk_kakutei_text_euc != NULL)
      {
	g_free (chunk_kakutei_text_euc);
      }
    if (chunk_muhenkan_text_euc != NULL)
      {
	g_free (chunk_muhenkan_text_euc);
      }
    
    jrKanjiString(cn->canna_context, 0x06, buffer, BUFSIZ, &cn->ks); /* Ctrl+F */
  } while ( cn->ks.revPos > 0 && cn->ks.revLen > 0 );

  furigana_slist = discard_strict_furigana(finalstr, furigana_slist);
  g_free(finalstr);
   
  if (0) { /* Dump furigana slist */
    int i = 0;
    for(i=0;i<g_slist_length(furigana_slist);i++) {
      Furigana* f = g_slist_nth_data(furigana_slist, i);
      g_print("IM_CANNA: furigana[%d]=\"%s\", pos=%d, len=%d\n",
	i, f->text, f->offset, f->length);
    }
  }

  g_object_set_data(G_OBJECT(cn), "furigana", furigana_slist);
  
  return furigana_slist;
}

/***/
static gint
im_canna_get_utf8_len_from_euc (guchar *text_euc)
{
  gchar *text_utf8;
  gint utf8_len;

  text_utf8 = euc2utf8 (text_euc);

  utf8_len = g_utf8_strlen (text_utf8, -1);

  g_free (text_utf8);


  return utf8_len;
}

static gint
im_canna_get_utf8_pos_from_euc_pos (guchar *text_euc,
				    gint    pos)
{
  gchar *tmp_euc;
  gchar *tmp_utf8;
  gint utf8_pos;

  tmp_euc = g_strndup (text_euc, pos);
  tmp_utf8 = euc2utf8 (tmp_euc);

  utf8_pos = g_utf8_strlen (tmp_utf8, -1);

  g_free (tmp_euc);
  g_free (tmp_utf8);


  return utf8_pos;
}

/***/
static gboolean
im_canna_is_euc_char(guchar* p) {
  if ((p != NULL)      &&
      (*(p+0) != '\0') &&
      (*(p+1) != '\0') &&
      (*(p+0) & 0x80) &&
      (*(p+1) & 0x80))
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}
/* Checking Hiragana in EUC-JP
 *  212b, 212c, 213c, 2421-2473 (JIS)
 *  A1AB, A1AC, A1BC, A4A1-A4F3 (EUC)
 */
static gboolean
im_canna_is_euc_hiragana_char(guchar* p) {
  if ((p == NULL)      ||
      (*(p+0) == '\0') ||
      (*(p+1) == '\0'))
    {
      return FALSE;
    }
  else
    {
      guint16 mb;

      mb = ((*(p+0) << 8) |
	    (*(p+1) << 0));

      if ((mb == 0xA1AB) ||
	  (mb == 0xA1AC) ||
	  (mb == 0xA1BC) ||
	  ((mb >= 0xA4A1) && (mb <= 0xA4F3)))
	{
	  return TRUE;
	}
      else
	{
	  return FALSE;
	}
    }
}

static gboolean
im_canna_is_euc_hiragana(guchar* text) {
  guchar *p = text;

  if( text == NULL || *text == '\0' )
    return FALSE;

  if(strlen(p) < 2)
    return FALSE;

  while(strlen(p) >= 2) {
    if (im_canna_is_euc_hiragana_char (p) == FALSE) {
      return FALSE;
    }

    p += 2;

    if( strlen(p) == 1 )
      return FALSE;

    if( strlen(p) == 0 )
      return TRUE;
  }

  return FALSE;
}

/* Checking Kanji in EUC-JP
 *  (rough area for faster calc, and jisx0208 only)
 *  3021-7424 (JIS)
 *  B0A1-F4A4 (EUC)
 */
static gboolean
im_canna_is_euc_kanji(guchar* text) {
  guchar *p = text;

  if( text == NULL || *text == '\0' )
    return FALSE;

  if(strlen(p) < 2)
    return FALSE;

  while(strlen(p) >= 2) {
    guint16 mb = (p[0]<<8) + p[1];

    if( !(mb >= 0xB0A1 && mb <= 0xF4A4) ) {
      return FALSE;
    }

    p += 2;

    if( strlen(p) == 1 )
      return FALSE;

    if( strlen(p) == 0 )
      return TRUE;
  }

  return FALSE;
}

/***/
static gint
im_canna_get_pre_hiragana_len (guchar* text)
{
  gint pre_hiragana_len;
  guchar *p;
  int text_len;
  int i;


  pre_hiragana_len = 0;

  text_len = strlen (text);

  i = 0;
  while (i < text_len)
    {
      if ((text_len - i) < 2)
	{
	  break;
	}

      p = (text + i);

      if (im_canna_is_euc_hiragana_char (p) == TRUE)
	{
	  i += 2;

	  pre_hiragana_len += 2;
	}
      else
	{
	  break;
	}
    }


  return pre_hiragana_len;
}

static gint
im_canna_get_post_hiragana_len (guchar* text)
{
  gint post_hiragana_len;
  guchar *p;
  guchar *tmp;
  int text_len;
  int i;


  tmp = NULL;
  text_len = strlen (text);

  i = 0;
  while (i < text_len)
    {
      if ((text_len - i) < 2)
	{
	  tmp = NULL;
	  break;
	}

      p = (text + i);

      if (im_canna_is_euc_hiragana_char (p) == TRUE)
	{
	  if (tmp == NULL)
	    {
	      tmp = p;
	    }

	  i += 2;
	}
      else
	{
	  tmp = NULL;

	  if (im_canna_is_euc_char (p) == TRUE)
	    {
	      i += 2;
	    }
	  else
	    {
	      i += 1;
	    }
	}
    }

  if (tmp != NULL)
    {
      post_hiragana_len = ((text + text_len) - tmp);
    }
  else
    {
      post_hiragana_len = 0;
    }


  return post_hiragana_len;
}

static GSList *
im_canna_get_furigana_slist (gchar *kakutei_text_euc,
			     gchar *muhenkan_text_euc)
{
  GSList *furigana_slist;
  gint kakutei_text_euc_len;
  gint muhenkan_text_euc_len;
  gint pre_hiragana_len;
  gint post_hiragana_len;
  gboolean as_is;


  furigana_slist = NULL;

  pre_hiragana_len  = im_canna_get_pre_hiragana_len  (kakutei_text_euc);
  post_hiragana_len = im_canna_get_post_hiragana_len (kakutei_text_euc);

  kakutei_text_euc_len  = strlen (kakutei_text_euc);
  muhenkan_text_euc_len = strlen (muhenkan_text_euc);

  as_is = FALSE;

  if ((as_is == FALSE) &&
      ((pre_hiragana_len + post_hiragana_len) > muhenkan_text_euc_len))
    {
      as_is = TRUE;
    }

  if ((as_is == FALSE) &&
      (pre_hiragana_len > 0))
    {
      if ((strncmp (kakutei_text_euc,
		    muhenkan_text_euc,
		    pre_hiragana_len) != 0))
	{
	  as_is = TRUE;
	}
    }

  if ((as_is == FALSE) &&
      (post_hiragana_len > 0))
    {
      if ((strncmp (((kakutei_text_euc
		      + kakutei_text_euc_len)
		     - (post_hiragana_len)),
		    ((muhenkan_text_euc
		      + muhenkan_text_euc_len)
		     - (post_hiragana_len)),
		    post_hiragana_len) != 0))
	{
	  as_is = TRUE;
	}
    }

  if (as_is == FALSE)
    {
      gchar *tmp_kakutei_text_euc;
      gchar *tmp_muhenkan_text_euc;
      gint   tmp_kakutei_text_euc_len;
      gint   tmp_muhenkan_text_euc_len;

      tmp_kakutei_text_euc
	= g_strndup ((kakutei_text_euc + pre_hiragana_len),
		     (kakutei_text_euc_len
		      - pre_hiragana_len
		      - post_hiragana_len));
      tmp_muhenkan_text_euc
	= g_strndup ((muhenkan_text_euc + pre_hiragana_len),
		     (muhenkan_text_euc_len
		      - pre_hiragana_len
		      - post_hiragana_len));

      tmp_kakutei_text_euc_len  = strlen (tmp_kakutei_text_euc);
      tmp_muhenkan_text_euc_len = strlen (tmp_muhenkan_text_euc);

      {
	struct _Hiragana {
	  gint pos;
	  gint len;

	  gint pos2;
	};
	typedef struct _Hiragana Hiragana;

	GList *hiragana_list;
	GList *tmp;
	Hiragana *hiragana;
	int i;
	gchar *p;

	hiragana_list = NULL;
	hiragana = NULL;

	i = 0;
	while (i < tmp_kakutei_text_euc_len)
	  {
	    if ((tmp_kakutei_text_euc_len - i) < 2)
	      {
		if (hiragana != NULL)
		  {
		    hiragana->len = (i - hiragana->pos);
		    hiragana = NULL;
		  }

		break;
	      }

	    p = (tmp_kakutei_text_euc + i);

	    if (im_canna_is_euc_hiragana_char (p) == TRUE)
	      {
		if (hiragana == NULL)
		  {
		    hiragana = g_new0 (Hiragana, 1);

		    hiragana->pos = i;
		    hiragana_list = g_list_append (hiragana_list, hiragana);
		  }

		i += 2;
	      }
	    else
	      {
		if (hiragana != NULL)
		  {
		    hiragana->len = (i - hiragana->pos);
		    hiragana = NULL;
		  }

		if (im_canna_is_euc_char (p) == TRUE)
		  {
		    i += 2;
		  }
		else
		  {
		    i += 1;
		  }
	      }
	  }

	if (hiragana_list != NULL)
	  {
	    GList *tmp2;
	    Hiragana *h;
	    gint l1;
	    gint l2;
	    gboolean match;
	    gchar *p;

	    tmp = hiragana_list;
	    
	    while (tmp != NULL)
	      {
		hiragana = (Hiragana *)tmp->data;
		tmp = g_list_next (tmp);

		l1 = 0;
		tmp2 = g_list_previous (tmp);
		while (tmp2 != NULL)
		  {
		    h = (Hiragana *)tmp2->data;
		    tmp2 = g_list_previous (tmp2);

		    l1 += h->len;
		  }

		l2 = 0;
		tmp2 = g_list_next (tmp);
		while (tmp2 != NULL)
		  {
		    h = (Hiragana *)tmp2->data;
		    tmp2 = g_list_next (tmp2);

		    l2 += h->len;
		  }

		match = FALSE;
		i = 0;
		while (i < (tmp_muhenkan_text_euc_len - l1 - l2 ))
		  {
		    if (((tmp_muhenkan_text_euc_len - l1 - l2) - i)
			< (hiragana->len))
		      {
			if (match == FALSE)
			  {
			    as_is = TRUE;
			  }
			break;
		      }

		    p = (tmp_muhenkan_text_euc + l1 + i);

		    if (im_canna_is_euc_char (p) == TRUE)
		      {
			if (strncmp (p,
				     (tmp_kakutei_text_euc + hiragana->pos),
				     hiragana->len)
			    == 0)
			  {
			    if (match == FALSE)
			      {
				match = TRUE;
				hiragana->pos2 = (l1 + i);
			      }
			    else
			      {
				as_is = TRUE;
				break;
			      }
			  }
			
			i += 2;
		      }
		    else
		      {
			i += 1;
		      }
		  }

		if (match == FALSE)
		  {
		    as_is = TRUE;
		    break;
		  }
	      }

	    if (as_is == FALSE)
	      {
		Furigana *furigana;
		gint pos;
		gint pos2;
		gchar *tmp_euc;

		pos = 0;
		pos2 = 0;

		{
		  hiragana = g_new0 (Hiragana, 1);

		  hiragana->pos = tmp_kakutei_text_euc_len;
		  hiragana->len = 0;
		  hiragana->pos2 = tmp_muhenkan_text_euc_len;

		  hiragana_list = g_list_append (hiragana_list, hiragana);
		}

		tmp = hiragana_list;
		while (tmp != NULL)
		  {
		    hiragana = (Hiragana *)tmp->data;
		    tmp = g_list_next (tmp);

		    furigana = g_new0 (Furigana, 1);

		    {
		      gint p1;
		      gint p2;

		      p1 = im_canna_get_utf8_pos_from_euc_pos
				(kakutei_text_euc,
				 (pre_hiragana_len +pos));
		      p2 = im_canna_get_utf8_pos_from_euc_pos
				(kakutei_text_euc,
				 (pre_hiragana_len + hiragana->pos));

		      pos = (hiragana->pos + hiragana->len);

		      furigana->offset = p1;
		      furigana->length = (p2 - p1);
		    }

		    {
		      gchar *tmp_utf8;
		      
		      tmp_euc = g_strndup ((tmp_muhenkan_text_euc + pos2),
					   (hiragana->pos2 - pos2));
		      tmp_utf8 = euc2utf8 (tmp_euc);
		      g_free (tmp_euc);

		      pos2 = (hiragana->pos2 + hiragana->len);

		      furigana->text = tmp_utf8;
		    }

		    furigana_slist = g_slist_append(furigana_slist, furigana);
		  }
	      }


	    g_list_foreach (hiragana_list, (GFunc)g_free, NULL);
	    g_list_free (hiragana_list);
	  }
	else
	  {
	    Furigana *furigana;
	    gchar *text_utf8;
	    gint length;
	    gint offset;

	    
	    text_utf8 = euc2utf8 (tmp_muhenkan_text_euc);
	    length = im_canna_get_utf8_len_from_euc (tmp_kakutei_text_euc);
	    offset = im_canna_get_utf8_pos_from_euc_pos (kakutei_text_euc,
							 pre_hiragana_len);


	    furigana = g_new0 (Furigana, 1);

	    furigana->text   = text_utf8;
	    furigana->length = length;
	    furigana->offset = offset;

	    furigana_slist = g_slist_append(furigana_slist, furigana);
	  }
      }
      
      g_free (tmp_kakutei_text_euc);
      g_free (tmp_muhenkan_text_euc);
    }

  if (as_is == TRUE)
    {
      Furigana *furigana;
      gchar *text_utf8;
      gchar *tmp_utf8;


      text_utf8 = euc2utf8 (muhenkan_text_euc);
      tmp_utf8  = euc2utf8 (kakutei_text_euc);


      furigana = g_new0 (Furigana, 1);

      furigana->text   = text_utf8;
      furigana->length = g_utf8_strlen (tmp_utf8, -1);
      furigana->offset = 0;


      g_free (tmp_utf8);


      furigana_slist = g_slist_append(furigana_slist, furigana);
    }

  return furigana_slist;
}
/***/

static void
im_canna_focus_in (GtkIMContext* context) {
  IMContextCanna* cn = IM_CONTEXT_CANNA(context);

  focused_context = context;

  im_canna_kill(cn);

  if( cn->focus_in_candwin_show == FALSE ) {
    im_canna_update_modewin(cn);
    return;
  }

  gtk_widget_show_all(cn->candwin);
  im_canna_update_modewin(cn);

}

static void
im_canna_focus_out (GtkIMContext* context) {
  IMContextCanna* cn = IM_CONTEXT_CANNA(context);

  focused_context = NULL;

  im_canna_reset (context);

/*   if( cn->kslength >= 1) { */
/*     gchar* eucstr = g_strndup(cn->ks.echoStr, cn->kslength); */
/*     gchar* utf8 = euc2utf8(eucstr); */
/*     g_signal_emit_by_name(cn, "commit", utf8); */
/*     g_free(utf8); */
/*     im_canna_force_to_kana_mode(cn); */
/*     g_signal_emit_by_name(cn, "preedit_changed"); */
/*   } */

  gtk_widget_hide_all(GTK_WIDGET(cn->modewin));
}

static void
im_canna_set_cursor_location (GtkIMContext *context, GdkRectangle *area)
{
  IMContextCanna *cn = IM_CONTEXT_CANNA(context);
  GdkScreen* screen;
  gint scr_width, scr_height;
  gint x, y, candwin_width, candwin_height;

  gdk_window_get_origin(cn->client_window, &x, &y);
  gtk_window_get_size(GTK_WINDOW(cn->candwin),
		      &candwin_width, &candwin_height);

  screen = gdk_drawable_get_screen (cn->client_window);
  scr_width  = gdk_screen_get_width  (screen);
  scr_height = gdk_screen_get_height (screen);

  cn->candwin_pos_x = x + area->x - candwin_width / 2;
  if (cn->candwin_pos_x < 0)
    cn->candwin_pos_x = 0;
  else if (cn->candwin_pos_x + candwin_width > scr_width)
    cn->candwin_pos_x = scr_width - candwin_width;
 
  cn->candwin_pos_y = y + area->y + area->height;
  if (cn->candwin_pos_y + candwin_height > scr_height)
    cn->candwin_pos_y = scr_height - candwin_height;

  gtk_window_move(GTK_WINDOW(cn->candwin),
		  cn->candwin_pos_x,
		  cn->candwin_pos_y);
}

static void
im_canna_move_window(IMContextCanna* cn, GtkWidget* widget) {
  GdkWindow* toplevel_gdk = cn->client_window;
  GdkScreen* screen;
  GdkWindow* root_window;
  GtkWidget* toplevel;
  GdkRectangle rect;
  GtkRequisition requisition;
  gint y;
  gint height;

  if( !toplevel_gdk )
    return;

  screen = gdk_drawable_get_screen (toplevel_gdk);
  root_window = gdk_screen_get_root_window(screen);

  while(TRUE) {
    GdkWindow* parent = gdk_window_get_parent(toplevel_gdk);
    if( parent == root_window )
      break;
    else
      toplevel_gdk = parent;
  }

  if (widget == cn->candwin) {
    gtk_window_move (GTK_WINDOW (widget),
		     cn->candwin_pos_x,
		     cn->candwin_pos_y);
    return;
  }

  /* GdkWindow's user data is traditionally GtkWidget pointer owns it.
   *
   */ 
  gdk_window_get_user_data(toplevel_gdk, (gpointer*)&toplevel);
  if( !toplevel )
    return;

  height = gdk_screen_get_height (gtk_widget_get_screen (toplevel));
  gdk_window_get_frame_extents (toplevel->window, &rect);
  gtk_widget_size_request (widget, &requisition);

  if (rect.y + rect.height + requisition.height < height)
    y = rect.y + rect.height;
  else
    y = height - requisition.height;

  gtk_window_move (GTK_WINDOW (widget), rect.x, y);
}

static void
im_canna_update_modewin(IMContextCanna* cn) {
  int len = 0;
  gchar* modebuf = NULL;
  gchar* modebuf_utf8 = NULL;

  PangoAttrList* attrs;
  PangoAttribute* attr;

  gboolean imsss_ret;

  /*
     Hide when the candidate window is up.
     The mode window should play the secondary role while the candidate
     window plays the leading role, because candidate window has more
     important info for user input.
   */
  if( GTK_WIDGET_VISIBLE(cn->candwin) ) {
    /* gtk_widget_hide_all(cn->modewin); */
    return;
  }

  if( !cn->ja_input_mode ) {
    imsss_set_status(IMSSS_STATUS_NONE);
    gtk_widget_hide_all(cn->modewin);
    return;
  }

  len = jrKanjiControl(cn->canna_context, KC_QUERYMAXMODESTR, 0);
  modebuf = g_new0(gchar, len+1);
  jrKanjiControl(cn->canna_context, KC_QUERYMODE, modebuf);

  if( !strcmp(modebuf, CANNA_MODESTR_NORMAL) ) {
    imsss_ret = imsss_set_status(IMSSS_STATUS_KANA);
  } else {
    imsss_ret = imsss_set_status(IMSSS_STATUS_KANJI);
  }
  if( imsss_ret ) {
    g_free(modebuf);
    return;
  }

  modebuf_utf8 = euc2utf8(modebuf);

  gtk_label_set_label(GTK_LABEL(cn->modelabel), modebuf_utf8);

  /* Set Mode Window Background Color to Blue */
  attrs = gtk_label_get_attributes(GTK_LABEL(cn->modelabel));
  if( attrs ) {
    pango_attr_list_unref(attrs);
  }

  attrs = pango_attr_list_new();
  attr = pango_attr_background_new(0xDB00, 0xE900, 0xFF00);
  attr->start_index = 0;
  attr->end_index = strlen(modebuf_utf8);
  pango_attr_list_insert(attrs, attr);
  
  gtk_label_set_attributes(GTK_LABEL(cn->modelabel), attrs);
  /* - Coloring Done - */

  im_canna_move_window(cn, cn->modewin);
  gtk_widget_show_all(cn->modewin);

  g_free(modebuf_utf8);
  g_free(modebuf);
}

static void
im_canna_update_candwin(IMContextCanna* cn) {
  if( cn->ks.info & KanjiModeInfo ) {
    im_canna_update_modewin(cn);
  }
  if ( cn->ks.info & KanjiGLineInfo ) {
/*     printf("GLineInfo ks.mode: %s\n", cn->ks.mode); */
/*     printf("GLineInfo ks.gline.line: %s\n", cn->ks.gline.line); */
/*     printf("GLineInfo ks.gline.length: %d\n", cn->ks.gline.length); */

    if( cn->ks.gline.length == 0 ) {
      im_canna_hide_candwin(cn);
      im_canna_update_modewin(cn); /* Expect modewin will be up */
    } else {
      im_canna_show_candwin(cn, cn->ks.gline.line);
    }
  }

}

static guint
get_canna_keysym(guint keyval, guint state) {
  guint i = 0;

  while( gdk2canna_keytable[i].gdk_keycode != 0
         && gdk2canna_keytable[i].canna_keycode != 0 ) {
    guint mask = gdk2canna_keytable[i].mask;
    if( (state & GDK_CONTROL_MASK) == (mask & GDK_CONTROL_MASK)
        && (state & GDK_SHIFT_MASK) == (mask & GDK_SHIFT_MASK)
        && gdk2canna_keytable[i].gdk_keycode == keyval ) {
      return gdk2canna_keytable[i].canna_keycode;
    }

    i++;
  }

  return keyval;
}

static gboolean
return_false(GtkIMContext* context, GdkEventKey* key) {
  return FALSE;
}

/* im_canna_reset() commit all preedit string and clear the buffers.
   It should not reset input mode.

   Applications need to call gtk_im_context_reset() when they get
   focus out event. And when each editable widget losed focus, apps
   need to call gtk_im_context_reset() too to commit the string.
 */
static void
im_canna_reset(GtkIMContext* context) {
  IMContextCanna* cn = (IMContextCanna*)context;

  if( cn->kslength ) {
    gchar* utf8 = NULL;
    memset(cn->workbuf, 0, BUFSIZ);
    strncpy(cn->workbuf, cn->ks.echoStr, cn->kslength);
    utf8 = euc2utf8(cn->workbuf);
    g_signal_emit_by_name(cn, "commit", utf8);
    cn->kslength = 0;
    g_free(utf8);
  } 

  memset(cn->workbuf, 0, BUFSIZ);
  memset(cn->kakutei_buf, 0, BUFSIZ);
  g_signal_emit_by_name(cn, "preedit_changed");
}

/* im_canna_kakutei() just do kakutei, commit and flush.
 * This works for generic canna usage, but doesn't for furigana support.
 */
static void
im_canna_kakutei(IMContextCanna* cn) {
  jrKanjiStatusWithValue ksv;
  guchar buf[BUFSIZ];
  int len = 0;
  gchar* utf8 = NULL;

  ksv.ks = &cn->ks;
  ksv.buffer = buf;
  ksv.bytes_buffer = BUFSIZ;

  len = jrKanjiControl(cn->canna_context, KC_KAKUTEI, (void*)&ksv);
  utf8 = euc2utf8(buf);

  g_signal_emit_by_name(cn, "commit", utf8);
  memset(cn->workbuf, 0, BUFSIZ);
  memset(cn->kakutei_buf, 0, BUFSIZ);
  g_signal_emit_by_name(cn, "preedit_changed");

  g_free(utf8);
}

static void
im_canna_kill(IMContextCanna* cn) {
  jrKanjiStatusWithValue ksv;
  guchar buf[BUFSIZ];
  int len = 0;
  gchar* utf8 = NULL;
    
  ksv.ks = &cn->ks;
  ksv.buffer = buf;
  ksv.bytes_buffer = BUFSIZ;
    
  len = jrKanjiControl(cn->canna_context, KC_KILL, (void*)&ksv);
  utf8 = euc2utf8(buf);
  
  memset(cn->workbuf, 0, BUFSIZ);
  memset(cn->kakutei_buf, 0, BUFSIZ);
  g_signal_emit_by_name(cn, "preedit_changed");

  jrKanjiControl(cn->canna_context, KC_INITIALIZE, 0);

  g_free(utf8);
}
