
/******************************************************
 *
 * canvasposition - implementation file
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
 * this object provides a way to get and set the position of the containing
 * abstraction within the parent-patch
 *
 * by default the position of the containing abstraction within the parent-patch is
 * queried
 * you can give the "depth" as argument;
 * e.g. [canvasobjectposition 1] will set/get the position of the abstraction containing the
 * abstraction within its canvas.
 */

#include "iemguts.h"

#include "g_canvas.h"
#include "m_imp.h"

/* ------------------------- canvasobjectposition ---------------------------- */

static t_class *canvasobjectposition_class;

typedef struct _canvasobjectposition
{
  t_object  x_obj;
  t_canvas *x_parent;   // the canvas we are acting on
  int       x_index;

  t_outlet*x_posoutlet, *x_sizeoutlet, *x_extraoutlet;

} t_canvasobjectposition;

static t_object* index2obj(t_canvas*glist, int index)
{
  t_gobj*gobj=NULL;
  int i=index;
  if(glist==NULL) return NULL;
  if(index<0) return NULL;
  if(NULL==glist) return NULL;

  gobj=glist->gl_list;
  while(i-- && gobj) {
    gobj=gobj->g_next;
  }
  return pd_checkobject(&gobj->g_pd);
}
static void canvasobjectposition_index(t_canvasobjectposition *x, t_float findex)
{
  int index=findex;
  if(index2obj(x->x_parent, index))
    x->x_index = index;
  else
    pd_error(x, "object index %d out of range", index);
}

static void canvasobjectposition_bang(t_canvasobjectposition *x)
{
  t_object*obj=index2obj(x->x_parent, x->x_index);
  t_canvas*cnv=x->x_parent;
  t_float zoom=1.;
  t_float x1=0, y1=0, width=0, height=0;
  t_atom alist[2];

  if(!obj) return;

  x1=obj->te_xpix;
  y1=obj->te_ypix;

  if(cnv) {
    width= (cnv->gl_screenx2 - cnv->gl_screenx1);
    height=(cnv->gl_screeny2 - cnv->gl_screeny1);
#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 47)
    if(iemguts_check_atleast_pdversion(0,47,0)) {
      zoom = cnv->gl_zoom;
    }
#endif
  }

  SETFLOAT(alist+0, zoom);
  outlet_anything(x->x_extraoutlet, gensym("zoom"), 1, alist);

  SETFLOAT(alist+0, width);
  SETFLOAT(alist+1, height);
  outlet_list(x->x_sizeoutlet, 0, 2, alist);

  SETFLOAT(alist+0, x1/zoom);
  SETFLOAT(alist+1, y1/zoom);
  outlet_list(x->x_posoutlet, 0, 2, alist);
}

static void canvasobjectposition_list(t_canvasobjectposition *x, t_symbol*s, int argc, t_atom*argv)
{
  t_object*obj=index2obj(x->x_parent, x->x_index);
  t_canvas*cnv=x->x_parent;
  int dx, dy;
  t_float zoom = 1.;

  if(!obj) return;

  if(argc==0){
    canvasobjectposition_bang(x);
    return;
  }

  if(argc!=2 || (A_FLOAT != (argv+0)->a_type) || (A_FLOAT != (argv+1)->a_type)) {
    pd_error(x, "expected <x> <y> as new position");
    return;
  }
#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 47)
  if(cnv && iemguts_check_atleast_pdversion(0,47,0)) {
    zoom = cnv->gl_zoom;
  }
#endif

  dx = atom_getfloat(argv+0)*zoom - obj->te_xpix;
  dy = atom_getfloat(argv+1)*zoom - obj->te_ypix;


  if(cnv&&glist_isvisible(cnv))  {
    gobj_displace(&obj->te_g, cnv, dx, dy);
    canvas_fixlinesfor(cnv, obj);
  } else {
    obj->te_xpix+=dx;
    obj->te_ypix+=dy;
  }
}

static void canvasobjectposition_free(t_canvasobjectposition *x)
{
  outlet_free(x->x_posoutlet);
  outlet_free(x->x_sizeoutlet);
  outlet_free(x->x_extraoutlet);
}

static void *canvasobjectposition_new(t_symbol*s, int argc, t_atom*argv)
{
  if(argc>2) {
    error("syntax: canvasobjectposition <canvasdepth> [<objectindex>]");
    return NULL;
  } else {
    t_canvasobjectposition *x = (t_canvasobjectposition *)pd_new(canvasobjectposition_class);
    t_glist *glist=(t_glist *)canvas_getcurrent();
    t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
    int index=-1;
    int depth=0;
#ifdef __GNUC__
# define CASE_FALLTHROUGH __attribute__ ((fallthrough))
#else
# define CASE_FALLTHROUGH
#endif
    switch(argc) {
    case 2:
      index=atom_getfloat(argv+1);
      CASE_FALLTHROUGH;
    case 1:
      depth=atom_getfloat(argv);
      CASE_FALLTHROUGH;
    default:
      break;
    }

    if(depth<0)depth=0;

    while(depth && canvas) {
      canvas=canvas->gl_owner;
      depth--;
    }

    x->x_parent = canvas;
    x->x_index = index;

    x->x_posoutlet=outlet_new(&x->x_obj, &s_list);
    x->x_sizeoutlet=outlet_new(&x->x_obj, &s_list);
    x->x_extraoutlet=outlet_new(&x->x_obj, 0);

    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("object"));

    return (x);
  }
  return NULL;
}

void canvasobjectposition_setup(void)
{
  iemguts_boilerplate("[canvasobjectposition]", 0);
  canvasobjectposition_class = class_new(gensym("canvasobjectposition"),
                                   (t_newmethod)canvasobjectposition_new, (t_method)canvasobjectposition_free,
                                   sizeof(t_canvasobjectposition), 0,
                                   A_GIMME, 0);
  class_addbang(canvasobjectposition_class, (t_method)canvasobjectposition_bang);
  class_addlist(canvasobjectposition_class, (t_method)canvasobjectposition_list);

  class_addmethod(canvasobjectposition_class, (t_method)canvasobjectposition_index, gensym("object"), A_FLOAT, 0);

#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION <= 0 && PD_MINOR_VERSION < 47)
  if(iemguts_check_atleast_pdversion(0,47,0)) {
    int got_major=0, got_minor=0, got_bugfix=0;
    sys_getversion(&got_major, &got_minor, &got_bugfix);
    error("[canvasobjectposition] disabled zoom support at compile-time");
  }
#endif
}
