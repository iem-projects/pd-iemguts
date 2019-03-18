/******************************************************
 *
 * alias - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2007:forum::für::umläute:2019
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 * this object renames internals
 * [objectrename dac~ _dac~] will rename the built-in [dac~] to [_dac~]. (and free the "dac~" name
 */

#include "iemguts.h"
#include "g_canvas.h"
#include "m_imp.h"

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- objectrename ---------------------------- */

static t_class *objectrename_class;

typedef struct _objectrename
{
  t_object  x_obj;
} t_objectrename;


typedef t_pd *(*t_newgimme)(t_symbol *s, int argc, t_atom *argv);

void objectrename_doit(t_symbol*from,t_symbol*to) {
  t_class*pom_class = pd_objectmaker;
  int i;
  for(i=0; i<pom_class->c_nmethod; i++) {
    if(from == pom_class->c_methods[i].me_name) {
      verbose(1, "renaming '%s' to '%s'", from->s_name, to->s_name);
      pom_class->c_methods[i].me_name = to;
      return;
    }
  }
  error("unable to rename '%s' (not found)", from->s_name);
}

static void *objectrename_new(t_symbol*from, t_symbol*to)
{
  t_pd*x=0;
  if(!pd_objectmaker) {
    error("[objectrename] could not find pd_objectmaker");
    return NULL;
  }
  objectrename_doit(from, to);
  x=pd_new(objectrename_class);
  return (x);
}

void objectrename_setup(void)
{
  iemguts_boilerplate("[objectrename]", 0);
  objectrename_class = class_new(gensym("objectrename"),
      (t_newmethod)objectrename_new, NULL,
      sizeof(t_objectrename), 0,
      A_SYMBOL, A_SYMBOL, 0);
}
