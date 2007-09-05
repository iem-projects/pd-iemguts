
/******************************************************
 *
 * propertybang - implementation file
 *
 * copyleft (c) IOhannes m zm-bölnig-A
 *
 *   2007:forum::f-bür::umläute:2007-A
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/


/* 
 * this object provides a way to make an abstraction "property" aware
 * usage:
 *   + put this object into an abstraction
 *   + put the abstraction in a patch
 *   + you can now right-click on the abstraction and select the "properties" menu
 *   + selecting the "properties" menu, will send a bang to the outlet of this object
 *
 * nice, eh?
 */


#include "m_pd.h"
#include "g_canvas.h"


/* ------------------------- propertybang ---------------------------- */

static t_class *propertybang_class;

typedef struct _propertybang
{
  t_object  x_obj;
  t_symbol *x_d0name;
} t_propertybang;


static void propertybang_free(t_propertybang *x)
{
  pd_unbind(&x->x_obj.ob_pd, x->x_d0name);
}

static void propertybang_anything(t_propertybang *x, t_symbol*s, int argc, t_atom*argv) {
  if(s==&s_bang && argc==0) {
    outlet_bang(x->x_obj.ob_outlet);
  }
}

static void propertybang_properties(t_gobj*z, t_glist*owner) {
  // argh: z is the abstraction! but we need to access ourselfs!
  // we handle this by binding to a special symbol. e.g. "$0-propertybang"

  t_symbol*s_d0name=canvas_realizedollar((t_canvas*)z, gensym("$0 propertybang"));
    pd_bang(s_d0name->s_thing);
}

static void *propertybang_new(void)
{
  t_propertybang *x = (t_propertybang *)pd_new(propertybang_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  t_class *class = ((t_gobj*)canvas)->g_pd;

  outlet_new(&x->x_obj, &s_bang);

  x->x_d0name=canvas_realizedollar(canvas, gensym("$0 propertybang"));
  pd_bind(&x->x_obj.ob_pd, x->x_d0name);

  class_setpropertiesfn(class, propertybang_properties);
  return (x);
}

void propertybang_setup(void)
{
  propertybang_class = class_new(gensym("propertybang"), (t_newmethod)propertybang_new,
    (t_method)propertybang_free, sizeof(t_propertybang), CLASS_NOINLET, 0);
  class_addanything(propertybang_class, propertybang_anything);
}
