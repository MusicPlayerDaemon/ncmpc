/*
 * This is a quick-and-dirty emulator of the nl_langinfo(CODESET)
 * function defined in the Single Unix Specification for those systems
 * (FreeBSD, etc.) that don't have one yet. It behaves as if it had
 * been called after setlocale(LC_CTYPE, ""), that is it looks at
 * the locale environment variables.
 *
 * http://www.opengroup.org/onlinepubs/7908799/xsh/langinfo.h.html
 *
 * Please extend it as needed and suggest improvements to the author.
 * This emulator will hopefully become redundant soon as
 * nl_langinfo(CODESET) becomes more widely implemented.
 *
 * Since the proposed Li18nux encoding name registry is still not mature,
 * the output follows the MIME registry where possible:
 *
 *   http://www.iana.org/assignments/character-sets
 *
 * A possible autoconf test for the availability of nl_langinfo(CODESET)
 * can be found in
 *
 *   http://www.cl.cam.ac.uk/~mgk25/unicode.html#activate
 *
 * Markus.Kuhn@cl.cam.ac.uk -- 2002-03-11
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version:
 *
 *   http://www.cl.cam.ac.uk/~mgk25/ucs/langinfo.c
 */

#include <stdlib.h>
#include <string.h>
#include "support.h"

#define C_CODESET "US-ASCII"     /* Return this as the encoding of the
				  * C/POSIX locale. Could as well one day
				  * become "UTF-8". */

#define digit(x) ((x) >= '0' && (x) <= '9')

static char buf[16];

char *nl_langinfo(nl_item item)
{
  char *l, *p;
  
  if (item != CODESET)
    return NULL;
  
  if (((l = getenv("LC_ALL"))   && *l) ||
      ((l = getenv("LC_CTYPE")) && *l) ||
      ((l = getenv("LANG"))     && *l)) {
    /* check standardized locales */
    if (!strcmp(l, "C") || !strcmp(l, "POSIX"))
      return C_CODESET;
    /* check for encoding name fragment */
    if (strstr(l, "UTF") || strstr(l, "utf"))
      return "UTF-8";
    if ((p = strstr(l, "8859-"))) {
      memcpy(buf, "ISO-8859-\0\0", 12);
      p += 5;
      if (digit(*p)) {
	buf[9] = *p++;
	if (digit(*p)) buf[10] = *p++;
	return buf;
      }
    }
    if (strstr(l, "KOI8-R")) return "KOI8-R";
    if (strstr(l, "KOI8-U")) return "KOI8-U";
    if (strstr(l, "620")) return "TIS-620";
    if (strstr(l, "2312")) return "GB2312";
    if (strstr(l, "HKSCS")) return "Big5HKSCS";   /* no MIME charset */
    if (strstr(l, "Big5") || strstr(l, "BIG5")) return "Big5";
    if (strstr(l, "GBK")) return "GBK";           /* no MIME charset */
    if (strstr(l, "18030")) return "GB18030";     /* no MIME charset */
    if (strstr(l, "Shift_JIS") || strstr(l, "SJIS")) return "Shift_JIS";
    /* check for conclusive modifier */
    if (strstr(l, "euro")) return "ISO-8859-15";
    /* check for language (and perhaps country) codes */
    if (strstr(l, "zh_TW")) return "Big5";
    if (strstr(l, "zh_HK")) return "Big5HKSCS";   /* no MIME charset */
    if (strstr(l, "zh")) return "GB2312";
    if (strstr(l, "ja")) return "EUC-JP";
    if (strstr(l, "ko")) return "EUC-KR";
    if (strstr(l, "ru")) return "KOI8-R";
    if (strstr(l, "uk")) return "KOI8-U";
    if (strstr(l, "pl") || strstr(l, "hr") ||
	strstr(l, "hu") || strstr(l, "cs") ||
	strstr(l, "sk") || strstr(l, "sl")) return "ISO-8859-2";
    if (strstr(l, "eo") || strstr(l, "mt")) return "ISO-8859-3";
    if (strstr(l, "el")) return "ISO-8859-7";
    if (strstr(l, "he")) return "ISO-8859-8";
    if (strstr(l, "tr")) return "ISO-8859-9";
    if (strstr(l, "th")) return "TIS-620";      /* or ISO-8859-11 */
    if (strstr(l, "lt")) return "ISO-8859-13";
    if (strstr(l, "cy")) return "ISO-8859-14";
    if (strstr(l, "ro")) return "ISO-8859-2";   /* or ISO-8859-16 */
    if (strstr(l, "am") || strstr(l, "vi")) return "UTF-8";
    /* Send me further rules if you like, but don't forget that we are
     * *only* interested in locale naming conventions on platforms
     * that do not already provide an nl_langinfo(CODESET) implementation. */
    return "ISO-8859-1"; /* should perhaps be "UTF-8" instead */
  }
  return C_CODESET;
}

/* For a demo, compile with "gcc -W -Wall -o langinfo -D TEST langinfo.c" */

#ifdef TEST
#include <stdio.h>
int main()
{
  printf("%s\n", nl_langinfo(CODESET));
  return 0;
}
#endif
