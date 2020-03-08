#include "chartype.h"

guint
gstr_starts_japanese_hiragana(gchar* str) {
  gunichar c;
  if( str == NULL || *str == '\0' )
    return 0;
  if( !g_utf8_validate(str, -1, NULL) )
    return 0;

  return 0;
}

guint
gstr_starts_japanese_katakana(gchar* str) {
  return 0;
}

guint
gstr_ends_japanese_hiragana(gchar* str) {
  return 0;
}

guint
gstr_ends_japanese_katakana(gchar* str) {
  return 0;
}
