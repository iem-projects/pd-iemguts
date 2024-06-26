
/******************************************************
 *
 * classtest - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2007:forum::für::umläute:2007
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 * this object provides a way to query Pd for existing classes
 */

#include "iemguts.h"
#include "g_canvas.h"

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- classtest ---------------------------- */

static t_class *classtest_class;

typedef struct _classtest
{
  t_object  x_obj;
  t_outlet *x_out;
} t_classtest;

static void classtest_symbol(t_classtest *x, t_symbol*s)
{
  t_float result=0.;
  if(!pd_objectmaker) {
    pd_error(x, "[classtest]: couldn't find pd_objectmaker!");
    return;
  }
  if(0!=zgetfn(&pd_objectmaker, s))
    result=1.;

  outlet_float(x->x_out, result);



}

static void classtest_free(t_classtest *x)
{
  outlet_free(x->x_out);
}

static void *classtest_new(void)
{
  t_classtest *x = (t_classtest *)pd_new(classtest_class);

  x->x_out = outlet_new(&x->x_obj, &s_float);
  return (x);
}

void classtest_setup(void)
{
  iemguts_boilerplate("[classtest]", 0);
  classtest_class = class_new(gensym("classtest"), (t_newmethod)classtest_new,
                               (t_method)classtest_free, sizeof(t_classtest), 0,
                              0);
  class_addsymbol(classtest_class, (t_method)classtest_symbol);
}
