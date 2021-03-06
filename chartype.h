/* Japanese Char Type detect functions */

#ifndef _CHARTYPE_H
#define _CHARTYPE_H

#include <glib.h>

gchar* utf8_hiragana[] = {
"ぁ",
"あ",
"ぃ",
"い",
"ぅ",
"う",
"ぇ",
"え",
"ぉ",
"お",
"か",
"が",
"き",
"ぎ",
"く",
"ぐ",
"け",
"げ",
"こ",
"ご",
"さ",
"ざ",
"し",
"じ",
"す",
"ず",
"せ",
"ぜ",
"そ",
"ぞ",
"た",
"だ",
"ち",
"ぢ",
"っ",
"つ",
"づ",
"て",
"で",
"と",
"ど",
"な",
"に",
"ぬ",
"ね",
"の",
"は",
"ば",
"ぱ",
"ひ",
"び",
"ぴ",
"ふ",
"ぶ",
"ぷ",
"へ",
"べ",
"ぺ",
"ほ",
"ぼ",
"ぽ",
"ま",
"み",
"む",
"め",
"も",
"ゃ",
"や",
"ゅ",
"ゆ",
"ょ",
"よ",
"ら",
"り",
"る",
"れ",
"ろ",
"ゎ",
"わ",
"ゐ",
"ゑ",
"を",
"ん",
"う゛",
"゛",
"゜",
"ヽ",
"ヾ",
"ゝ",
"ゞ",
"ー",
NULL,
};

/* Return val is the number of matched type chars */
guint gstr_starts_japanese_hiragana(gchar* str);
guint gstr_starts_japanese_katakana(gchar* str);
guint gstr_ends_japanese_hiragana(gchar* str);
guint gstr_ends_japanese_katakana(gchar* str);

#endif /* _CHARTYPE_H */
