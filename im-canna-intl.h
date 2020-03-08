#ifndef _IM_CANNA_INTL_H_
#define _IM_CANNA_INTL_H_

#define ENABLE_NLS 1

#ifdef ENABLE_NLS
#include<libintl.h>
#define _(String) dgettext(GETTEXT_PACKAGE,String)
#define P_(String) dgettext(GETTEXT_PACKAGE "-properties",String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else /* NLS is disabled */
#define _(String) (String)
#define P_(String) (String)
#define N_(String) (String)
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain) 
#endif

#endif
