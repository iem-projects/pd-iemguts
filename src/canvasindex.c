
/******************************************************
 *
 * canvasindex - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2007:forum::für::umläute:2007
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/


/* 
 * this object provides a way to get the position of the containing abstraction
 * within the parent-patch
 * this makes it easy to (dis)connect this abstraction to others
 */

#include "m_pd.h"
#include "g_canvas.h"

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- canvasindex ---------------------------- */

static t_class *canvasindex_class;

typedef struct _canvasindex
{
  t_object  x_obj;
  t_canvas  *x_canvas;

  t_outlet*xoutlet, *youtlet;
} t_canvasindex;

static void canvasindex_bang(t_canvasindex *x)
{
  t_canvas*c=x->x_canvas;
  t_canvas*c0=0;

  //  int index=-1;

  if(!c) return;
  c0=c->gl_owner;
  if(!c0)return;

  //  index=glist_getindex(c0, c);
  //  index=glist_getindex(c0, (t_gobj*)c);
  outlet_float(x->xoutlet, (t_float)(glist_getindex(c0, 0)));
  outlet_float(x->xoutlet, (t_float)(glist_getindex(c0, (t_gobj*)c)));
}

static void canvasindex_free(t_canvasindex *x)
{
  outlet_free(x->xoutlet);
  outlet_free(x->youtlet);
}

static void *canvasindex_new(void)
{
  t_canvasindex *x = (t_canvasindex *)pd_new(canvasindex_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);

  x->x_canvas = canvas;

  x->xoutlet=outlet_new(&x->x_obj, &s_float);
  x->youtlet=outlet_new(&x->x_obj, &s_float);

  return (x);
}

void canvasindex_setup(void)
{
  canvasindex_class = class_new(gensym("canvasindex"), (t_newmethod)canvasindex_new,
    (t_method)canvasindex_free, sizeof(t_canvasindex), 0, 0);
  class_addbang(canvasindex_class, (t_method)canvasindex_bang);
}
