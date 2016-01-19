/******************************************************
 *
 * iemguts - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2008:forum::für::umläute:2015
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/

/* generic include header for iemguts
 * includes other necessary files (m_pd.h)
 * and provides boilerplate functions/macros
 */

#ifndef INCLUDE_IEMGUTS_H_
#define INCLUDE_IEMGUTS_H_

#include "m_pd.h"

#ifdef STRINGIFY
# undef STRINGIFY
#endif
#define IEMGUTS_STRINGIFY_AUX(s) #s
#define STRINGIFY(s) IEMGUTS_STRINGIFY_AUX(s)

#ifndef BUILD_DATE
# define BUILD_DATE "on " __DATE__ " at " __TIME__
#endif

/**
 * print some boilerplate about when the external was compiled
 * and against which version of Pd
 */
static void iemguts_boilerplate(const char*name, const char*copyright) {
  const int v=0;
  if(name && *name) {
#ifdef VERSION
    verbose(v, "%s " STRINGIFY(VERSION), name);
#else
    verbose(v, "%s", name);
#endif
    if(copyright && *copyright) {
      verbose(v, "\t© %s", copyright);
    }

  verbose(v, "\tcompiled "BUILD_DATE);
  if(*PD_TEST_VERSION)
    verbose(v, "\t         against Pd version %d.%d-%d (%s)",
	 PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION,
	 PD_TEST_VERSION);
  else
    verbose(v, "\t         against Pd version %d.%d-%d",
	 PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION);
  }
}

#endif /* INCLUDE_IEMGUTS_H_ */
