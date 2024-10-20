
/******************************************************
 *
 * canvasdelete - implementation file
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
 * this object deletes itself (or the specified parent canvas) when banged
 */

#include "iemguts.h"

#include "g_canvas.h"
#include <stdlib.h>

/* ------------------------- canvasdelete ---------------------------- */

static t_class *canvasdelete_class;

typedef struct _canvasdelete
{
  t_object  x_obj;

  t_glist*x_glist;
  t_gobj *x_gobj;

  t_clock  *x_clock;
} t_canvasdelete;
static void canvasdelete_doit(t_canvasdelete *x)
{
  int dspstate= canvas_suspend_dsp();
  clock_free(x->x_clock);
  x->x_clock=NULL;

  if(x->x_glist)
    glist_delete(x->x_glist, x->x_gobj);
  else {
    t_atom ap[1];
    SETFLOAT(ap, 1);
    typedmess((t_pd*)(x->x_gobj), gensym("menuclose"), 1, ap);
  }


  canvas_resume_dsp(dspstate);
}

static void canvasdelete_bang(t_canvasdelete *x)
{
  if(x->x_clock) {
    pd_error(x, "deletion already scheduled");
    return;
  }
  if(NULL==x->x_gobj)return;

  x->x_clock=clock_new(x, (t_method)canvasdelete_doit);
  clock_delay(x->x_clock, 0);
}

static void *canvasdelete_new(t_floatarg f)
{
  t_canvasdelete *x = (t_canvasdelete *)pd_new(canvasdelete_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  t_gobj*obj=(t_gobj*)x;
  int depth=(int)f;

  if(depth<0)depth=0;

  while(depth && canvas) {
    obj=(t_gobj*)canvas;
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_glist = NULL;
  x->x_gobj = NULL;
  if (!depth) {
    x->x_glist = canvas;
    x->x_gobj=obj;
  }

  x->x_clock=NULL;

  return (x);
}

static void canvasdelete_free(t_canvasdelete*x)
{
  (void)x;
}

static void canvasdelete_canvasmethods(void);

void canvasdelete_setup(void)
{
  iemguts_boilerplate("[canvasdelete] - delete message for the canvas", 0);
  canvasdelete_class = class_new(gensym("canvasdelete"),
                                 (t_newmethod)canvasdelete_new, (t_method)canvasdelete_free,
                                 sizeof(t_canvasdelete), 0,
                                 A_DEFFLOAT, 0);
  class_addbang(canvasdelete_class, (t_method)canvasdelete_bang);

  canvasdelete_canvasmethods();
}


/* 'delete' message for the canvas */
static int canvas_delete_docb(t_glist*glist, t_gobj*wantobj) {
  /* this will crash Pd if the object to be deleted is on the stack
   * workarounds:
   *   - use a clock (see above)
   *   - finally fix this in Pd
   */
  t_gobj*obj=NULL;
  if(!glist || !wantobj)
    return 0;
  for(obj=glist->gl_list; obj; obj=obj->g_next) {
    if(wantobj == obj)
      break;
  }
  if(!obj)
    return 0;

  glist_delete(glist, obj);
  return 1;
}

static t_gobj**canvasdelete_indices2glist(const t_glist*glist, unsigned int argc, t_atom*argv) {
  t_gobj**result = malloc(argc*sizeof(*result));
  t_gobj**resptr=result;
  unsigned int i=0;
  for(i=0; i<argc; i++)result[i]=NULL;
  for(i=0; i<argc; i++) {
    int index=atom_getint(argv+i);
    t_gobj*obj=glist->gl_list;
    /* get handle to object */
    while(index-->0 && obj) {
      obj=obj->g_next;
    }
    if(obj) {
      *resptr++=obj;
    }
  }
  return result;
}

static void canvas_delete_cb(t_canvas*x, t_symbol*s, int argc, t_atom*argv)
{
  int dspstate= canvas_suspend_dsp();
  t_gobj**objs=canvasdelete_indices2glist(x, argc>0?argc:0, argv);
  int i;
  (void)s;
  for(i=0; i<argc; i++) {
    canvas_delete_docb(x, objs[i]);
  }
  free(objs);
  canvas_resume_dsp(dspstate);
}


static void canvasdelete_canvasmethods(void) {
  if(NULL==canvas_class)return;
  if(NULL==zgetfn(&canvas_class, gensym("delete"))) {
    iemguts_verbose(0, "adding 'delete' method to canvas");
    class_addmethod(canvas_class, (t_method)canvas_delete_cb, gensym("delete"), A_GIMME, 0);
  }
}
