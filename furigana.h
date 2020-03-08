#ifndef _FURIGANA_H
#define _FURIGANA_H

#include <glib.h>

typedef struct _Furigana {
  gchar* text; /* Furigana is in UTF-8 */
  gint foffset; /* Unused */
  gint offset; /* Offset is in UTF-8 chars, not bytes */
  gint length; /* Length is in UTF-8 chars, not bytes */
} Furigana;

void furigana_add_offset(GSList* furigana_slist, gint offsetdiff);

#endif /* _FURIGANA_H */
