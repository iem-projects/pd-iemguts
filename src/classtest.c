
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


/* urgh: very brittle... */
typedef struct _methodentry
{
    t_symbol *me_name;
    t_gotfn me_fun;
    unsigned char me_arg[MAXPDARG+1];
} t_methodentry;
typedef struct _klass {
  t_symbol *c_name;
  t_symbol *c_helpname;
  t_symbol *c_externdir;
  size_t c_size;
#ifdef PDINSTANCE
  t_methodentry **c_methods;
#else
  t_methodentry *c_methods;
#endif
  int c_nmethod;
  /* there's more here, but we are not interested... */
} t_klass;


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

static void classtest_bang(t_classtest *x) {
  if(!pd_objectmaker) {
    pd_error(x, "[classtest]: couldn't find pd_objectmaker!");
    return;
  }
  t_klass*k = (t_klass*)pd_objectmaker;
  t_methodentry*methods = 0;
#ifdef PDINSTANCE
  methods = k->c_methods[0];
#else
  methods = k->c_methods;
#endif
  for(int i=0; i<k->c_nmethod; i++) {
    outlet_symbol(x->x_out, methods[i].me_name);
  }
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
  class_addbang(classtest_class, (t_method)classtest_bang);
}
