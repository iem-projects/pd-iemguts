/******************************************************
 *
 * canvasargs - implementation file
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
 * this object provides a way to manipulate the parent-patches arguments (and name!)
 * usage:
 *   + put this object into an abstraction
 *   + put the abstraction in a patch
 *   + send the object a _list_ of arguments
 *    + the next time the patch (wherein the abstraction that holds this object lives)
 *      is saved, it will be saved with the new arguments instead of the old ones!
 *    - example: "list 2 3 4" will save the object as [<absname> 2 3 4]
 *   + you can also change the abstraction name itself by using a selector other than "list"
 *    - example: "bonkers 8 9" will save the object as [bonkers 8 9] regardless of it's original name
 *    - use with care!
 *
 * nice, eh?
 */

#include "iemguts.h"
#include "g_canvas.h"


/* ------------------------- canvasargs ---------------------------- */

static t_class *canvasargs_class;

typedef struct _canvasargs
{
  t_object  x_obj;

  t_canvas  *x_canvas;
} t_canvasargs;


static void canvasargs_list(t_canvasargs *x, t_symbol*s, int argc, t_atom*argv)
{
  t_binbuf*b=0;
  t_atom name[1];

  if(!x || !x->x_canvas) return;
  b=x->x_canvas->gl_obj.te_binbuf;

  if(!b)return;

  /* if this method is called with a non-special selector, we *rename* the object */
  if(s==0 || s==gensym("") || s==&s_list || s==&s_bang || s==&s_float || s==&s_symbol || s==&s_) {
    /* keep the given name */
    t_atom*ap=binbuf_getvec(b);
    s=atom_getsymbol(ap);
  }
  SETSYMBOL(name, s);

  binbuf_clear(b);
  binbuf_add(b, 1, name);
  binbuf_add(b, argc, argv);
}

static void canvasargs_doit(t_canvasargs *x, int expanddollargs)
{
  int argc=0;
  t_atom*argv=0;
  t_binbuf*b=0;

  if(!x->x_canvas) return;
  b=x->x_canvas->gl_obj.te_binbuf;
  if(b && !expanddollargs) {
    argc=binbuf_getnatom(b)-1;
    argv=binbuf_getvec(b)+1;
  } else {
    canvas_setcurrent(x->x_canvas);
    canvas_getargs(&argc, &argv);
    canvas_unsetcurrent(x->x_canvas);
  }

  if(argv)
    outlet_list(x->x_obj.ob_outlet, &s_list, argc, argv);
}

static void canvasargs_bang(t_canvasargs *x) {
  return canvasargs_doit(x, 1);
}
static void canvasargs_raw(t_canvasargs *x) {
  return canvasargs_doit(x, 0);
}



static void canvasargs_free(t_canvasargs *x)
{
  x->x_canvas = 0;
}

static void *canvasargs_new(t_floatarg f)
{
  t_canvasargs *x = (t_canvasargs *)pd_new(canvasargs_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);

  int depth=(int)f;
  if(depth<0)depth=0;

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_canvas = canvas;

  outlet_new(&x->x_obj, 0);
  return (x);
}

void canvasargs_setup(void)
{
  iemguts_boilerplate("[canvasargs]", 0);
  canvasargs_class = class_new(gensym("canvasargs"), (t_newmethod)canvasargs_new,
                               (t_method)canvasargs_free, sizeof(t_canvasargs), 0, A_DEFFLOAT, 0);
  class_addlist(canvasargs_class, (t_method)canvasargs_list);
  class_addbang(canvasargs_class, (t_method)canvasargs_bang);
  class_addmethod(canvasargs_class, (t_method)canvasargs_raw, gensym("raw"), 0);
}
