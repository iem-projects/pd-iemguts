
/******************************************************
 *
 * sendcanvasobject - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2022:forum::für::umläute:2022
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 * this object provides a way to send messages to objects on upstream canvases
 * by default it sends messages to objects on the containing canvas, but you
 * can give the "depth" as argument;
 * the target object can be given via the 2nd inlet or a 2nd argument
 * e.g. [sendcanvasobject 1] will send messages to the parent of the containing canvas
 */

#include "iemguts.h"
#include "g_canvas.h"

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- sendcanvasobject ---------------------------- */

static t_class *sendcanvasobject_class;

typedef struct _sendcanvasobject
{
  t_object  x_obj;
  t_canvas *x_parent;
  t_float   x_findex;
} t_sendcanvasobject;

static t_object* index2obj(t_canvas*glist, int index)
{
  t_gobj*gobj=NULL;
  int i=index;
  if(NULL==glist) return NULL;
  if(index<0) return NULL;

  gobj=glist->gl_list;
  while(i-- && gobj) {
    gobj=gobj->g_next;
  }
  if(!gobj)return 0;
  return pd_checkobject(&gobj->g_pd);
}

static void sendcanvasobject_anything(t_sendcanvasobject *x, t_symbol*s, int argc, t_atom*argv)
{
  t_object*object = 0;
  int index = (int)x->x_findex;
  if(index<0)
    return;
  object = index2obj(x->x_parent, index);
  if(!object)
    return;

  typedmess((t_pd*)object, s, argc, argv);
}

static void sendcanvasobject_free(t_sendcanvasobject *x)
{
  x->x_parent = 0;
}

static void *sendcanvasobject_donew(int depth, int index)
{
  t_sendcanvasobject *x = (t_sendcanvasobject *)pd_new(sendcanvasobject_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);

  floatinlet_new(&x->x_obj, &x->x_findex);

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_parent = canvas;
  x->x_findex = index;

  return (x);
}
static void *sendcanvasobject_new(t_symbol*s, int argc, t_atom*argv)
{
  if(argc>2) {
    pd_error(0, "syntax: %s <canvasdepth> [<objectindex>]", s->s_name);
    return NULL;
  } else {
    int depth=0, index=-1;
    switch(argc) {
    case 2:
      index=atom_getfloat(argv+1);
      /* falls through */
    case 1:
      depth=atom_getfloat(argv);
      /* falls through */
    default:
      break;
    }
    if(depth<0)depth=0;
    return sendcanvasobject_donew(depth, index);
  }
}


void sendcanvasobject_setup(void)
{
  iemguts_boilerplate("[sendcanvasobject]", 0);
  sendcanvasobject_class = class_new(gensym("sendcanvasobject"),
                               (t_newmethod)sendcanvasobject_new,
                               (t_method)sendcanvasobject_free,
                               sizeof(t_sendcanvasobject), 0,
                               A_GIMME, 0);
  class_addanything(sendcanvasobject_class, (t_method)sendcanvasobject_anything);
}
