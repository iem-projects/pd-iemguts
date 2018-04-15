
/******************************************************
 *
 * canvaslock - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2007:forum::für::umläute:2010
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 * this object locks an abstraction so it cannot be opened any more
 * usage:
 *   + put this object into an abstraction/subpatch
 *     the default is to lock the canvas, but you can override this by adding an argument '0'
 *   + click on the abstraction object - it will not open
 *   + send the a canvas a 'vis 1' message - it will not open
 *   ? right-click the object and select open - it will not open (TODO)
 *   + you can unlock the canvas by sending '0' to the [canvaslock] object.
 *
 * stupid, eh?
 */


#include "iemguts-objlist.h"
#include "g_canvas.h"


/* ------------------------- canvaslock ---------------------------- */

static t_class *canvaslock_class;

typedef struct _canvaslock
{
  t_object  x_obj;
  int x_locked;
} t_canvaslock;


t_propertiesfn s_orgfun=NULL;

static void canvaslock_free(t_canvaslock *x)
{
  removeObjectFromCanvases((t_pd*)x);
}

static void canvaslock_float(t_canvaslock *x, t_float f) {
  x->x_locked = (int)f;
}
static void canvaslock_vis(t_canvas *z, t_floatarg f) {
  t_iemguts_objlist*objs=objectsInCanvas((t_pd*)z);
  int locked = 0;
  if(objs) {
    t_canvaslock*x = (t_canvaslock*)(objs->obj);
    if(x)
      locked = x->x_locked;
  }
  if(!locked) {
    t_atom ap[1];
    SETFLOAT(ap, f);
    pd_typedmess((t_pd*)z, gensym("vis canvaslock"), 1, ap);
  }
}

static void canvaslock_menu_open(t_glist *z) {
  t_iemguts_objlist*objs=objectsInCanvas((t_pd*)z);
  int locked = 0;
  if(objs) {
    t_canvaslock*x = (t_canvaslock*)(objs->obj);
    if(x)
      locked = x->x_locked;
  }
  if(!locked) {
    pd_typedmess((t_pd*)z, gensym("menu-open canvaslock"), 0, 0);
  }
}

static void canvaslock_click(t_canvas *z,
    t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    canvaslock_vis(z, 1);
}



static void *canvaslock_donew(int locked) {
  t_canvaslock *x = (t_canvaslock *)pd_new(canvaslock_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  t_class *class = ((t_gobj*)canvas)->g_pd;
  t_propertiesfn orgfun=NULL;

  x->x_locked = locked;

  addObjectToCanvas((t_pd*)canvas, (t_pd*)x);
  return (x);
}

static void *canvaslock_new(t_symbol*s, int argc, t_atom*argv) {
  int locked = 1;
  if(argc) {
    if (1==argc && A_FLOAT == argv->a_type) {
      locked = !(!(int)atom_getfloat(argv));
    } else {
      pd_error(0, "bad arguments to [%s]", s->s_name);
      return 0;
    }
  }
  return canvaslock_donew(locked);
}

void canvaslock_setup(void)
{
  t_gotfn orgfun = 0;
  if(NULL==canvas_class)return;
  orgfun = zgetfn(&canvas_class, gensym("vis"));
  if((t_gotfn)canvaslock_vis != orgfun) {
    class_addmethod(canvas_class, (t_method)canvaslock_vis, gensym("vis"), A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)orgfun, gensym("vis canvaslock"), A_DEFFLOAT, 0);
    class_addmethod(canvas_class, (t_method)canvaslock_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);

    orgfun = zgetfn(&canvas_class, gensym("menu-open"));
    class_addmethod(canvas_class, (t_method)canvaslock_menu_open, gensym("menu-open"), A_NULL);
    class_addmethod(canvas_class, (t_method)orgfun, gensym("menu-open canvaslock"), A_NULL);

    iemguts_boilerplate("[canvaslock]", 0);
    canvaslock_class = class_new(gensym("canvaslock"), (t_newmethod)canvaslock_new,
                                 (t_method)canvaslock_free, sizeof(t_canvaslock), 0, A_GIMME, 0);
    class_addfloat(canvaslock_class, canvaslock_float);
  }
}
