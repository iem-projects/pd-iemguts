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

#ifndef BUILD_DATE
# define BUILD_DATE "on " __DATE__ " at " __TIME__
#endif

/**
 * print some boilerplate about when the external was compiled
 * and against which version of Pd
 */
static void iemguts_boilerplate_compiled(void) {
  post("\tcompiled "BUILD_DATE"");
  post("\t         against Pd version %d.%d-%d (%s)",
    PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION,
    PD_TEST_VERSION);
}

#endif /* INCLUDE_IEMGUTS_H_ */
