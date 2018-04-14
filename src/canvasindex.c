
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
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/* 
 * this object provides a way to get the position of the containing abstraction
 * within the parent-patch
 * this makes it easy to (dis)connect this abstraction to others
 *
 * by default the index of the containing abstraction within the parent-patch is 
 * queried; however you can give the "depth" as argument:
 * e.g. [canvasindex 1] will give you the index of the abstraction containing the
 * abstraction that holds this object
 */

#include "iemguts.h"
#include "m_imp.h"
#include "g_canvas.h"

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- canvasindex ---------------------------- */

static t_class *canvasindex_class;

typedef struct _canvasindex
{
  t_object x_obj;
  t_canvas *x_canvas;
  t_gobj   *x_self;
  t_outlet *xoutlet, *youtlet;
} t_canvasindex;

typedef struct _intlist
{
  int value;
  struct _intlist*next;
} t_intlist;

static void canvasindex_symbol(t_canvasindex *x, t_symbol*s)
{
  /* check whether an object of name <s> is in the canvas */
  t_gobj*y;
  int index=0;

  t_canvas*c=x->x_canvas;
  if(!c) return;

  for (y = (t_gobj*)c->gl_list; y; y = y->g_next) /* traverse all objects in canvas */
    {
      t_object*obj=(t_object*)y;
      t_class*obj_c=y->g_pd;
      t_symbol*cname=obj_c->c_name;
      t_binbuf*b=obj->te_binbuf;
      t_atom*ap=binbuf_getvec(b);
      int    ac=binbuf_getnatom(b);
      if(s!=cname && ac) {
	cname=atom_getsymbol(ap);
      }
      if(s==cname){
#warning LATER think about output format
	outlet_float(x->xoutlet, (t_float)index);
      }
      index++;
    }
}


static void canvasindex_float(t_canvasindex *x, t_floatarg f)
{
  /* get the objectname of object #<f> */
  int index=f, cur=0;
  t_gobj*y;

  t_canvas*c=x->x_canvas;
  if(index < 0 || !c) return;

  for (y = (t_gobj*)c->gl_list; y && cur<index; y = y->g_next) /* traverse all objects in canvas */
    {
      cur++;
    }
  if(y) {
      t_object*obj=(t_object*)y;
      t_binbuf*b=obj->te_binbuf;
      t_atom*ap=binbuf_getvec(b);
      int    ac=binbuf_getnatom(b);
      t_atom classatom[1];
      SETSYMBOL(classatom, y->g_pd->c_name);
      /* LATER: shan't we output the index of the object as well? */
      outlet_anything(x->youtlet, gensym("class"), 1, classatom);
      outlet_anything(x->xoutlet, gensym("binbuf"), ac, ap);
  }  
}

static void canvasindex_bang(t_canvasindex *x)
{
  t_gobj*c=x->x_self;
  t_canvas*c0=x->x_canvas;

  if(!c || !c0) return;

  outlet_float(x->youtlet, (t_float)(glist_getindex(c0, 0)));
  outlet_float(x->xoutlet, (t_float)(glist_getindex(c0, c)));
}

static void canvasindex_free(t_canvasindex *x)
{
  outlet_free(x->xoutlet);
  outlet_free(x->youtlet);
}

static void *canvasindex_new(t_floatarg f)
{
  t_canvasindex *x = (t_canvasindex *)pd_new(canvasindex_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);

  int depth=(int)f;
  x->x_self =(t_gobj*)x;
  x->x_canvas = canvas;
  if(depth>=0) {
    while(depth && canvas) {
      canvas=canvas->gl_owner;
      depth--;
    }
    x->x_self =(t_gobj*)canvas;
    x->x_canvas = canvas ? canvas->gl_owner : 0;
  }

  x->xoutlet=outlet_new(&x->x_obj, &s_float);
  x->youtlet=outlet_new(&x->x_obj, &s_float);

  return (x);
}

void canvasindex_setup(void)
{
  iemguts_boilerplate("[canvasindex]", 0);
  canvasindex_class = class_new(gensym("canvasindex"), 
                                (t_newmethod)canvasindex_new, (t_method)canvasindex_free, 
                                sizeof(t_canvasindex), 0, 
                                A_DEFFLOAT, 0);
  /* gets the index of the selected canvas and the number of objects in the container */
  class_addbang(canvasindex_class, (t_method)canvasindex_bang);
  /* gets the indices of the objects in the container that have the given name */
  class_addsymbol(canvasindex_class, (t_method)canvasindex_symbol);
  /* get the name, args and class of the object at the given index */
  class_addfloat(canvasindex_class, (t_method)canvasindex_float);
}
