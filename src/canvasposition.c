
/******************************************************
 *
 * canvasposition - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2007:forum::für::umläute:2018
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
 * e.g. [canvasposition 1] will set/get the position of the abstraction containing the
 * abstraction within its canvas.
 */

#include "iemguts.h"

#include "g_canvas.h"
#include "m_imp.h"

/* ------------------------- canvasposition ---------------------------- */

static t_class *canvasposition_class;

typedef struct _canvasposition
{
  t_object  x_obj;
  t_canvas  *x_canvas;

  t_outlet*posoutlet, *sizeoutlet, *extraoutlet;
} t_canvasposition;


static void canvasposition_bang(t_canvasposition *x)
{
  t_canvas*c=x->x_canvas;
  t_canvas*c0=0;
  t_float x1=0., y1=0., width=0., height=0.;
  t_float zoom = 1.;
  t_atom alist[2];

  if(!c) return;

  if((c0=c->gl_owner)) {
    width= (c0->gl_screenx2 - c0->gl_screenx1);
    height=(c0->gl_screeny2 - c0->gl_screeny1);

#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 47)
    if(iemguts_check_atleast_pdversion(0,47,0)) {
      zoom = c0->gl_zoom;
    }
#endif
  }

  x1=c->gl_obj.te_xpix;
  y1=c->gl_obj.te_ypix;

  SETFLOAT(alist+0, zoom);
  outlet_anything(x->extraoutlet, gensym("zoom"), 1, alist);

  SETFLOAT(alist+0, width);
  SETFLOAT(alist+1, height);
  outlet_list(x->sizeoutlet, 0, 2, alist);

  //  outlet_float(x->sizeoutlet, y1);
  SETFLOAT(alist+0, x1/zoom);
  SETFLOAT(alist+1, y1/zoom);
  outlet_list(x->posoutlet, 0, 2, alist);
}

static void canvasposition_list(t_canvasposition *x, t_symbol*s, int argc, t_atom*argv)
{
  t_canvas*c=x->x_canvas;
  t_canvas*c0=0;
  int dx, dy;
  t_float zoom = 1.;

  if(!c) return;
  c0=c->gl_owner;

  if(argc==0){
    canvasposition_bang(x);
    return;
  }

  if(argc!=2 || (A_FLOAT != (argv+0)->a_type) || (A_FLOAT != (argv+1)->a_type)) {
    pd_error(x, "expected <x> <y> as new position");
    return;
  }
#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 47)
  if(c0 && iemguts_check_atleast_pdversion(0,47,0)) {
    zoom = c0->gl_zoom;
  }
#endif

  dx = atom_getfloat(argv+0)*zoom - c->gl_obj.te_xpix;
  dy = atom_getfloat(argv+1)*zoom - c->gl_obj.te_ypix;

  if ((dx!=0)||(dy!=0)) {
    if(c0&&glist_isvisible(c0))  {
      gobj_displace((t_gobj*)c, c0, dx, dy);
      canvas_fixlinesfor(c0, (t_text*)c);
    } else {
      c->gl_obj.te_xpix+=dx;
      c->gl_obj.te_ypix+=dy;
    }
  }
}

static void canvasposition_free(t_canvasposition *x)
{
  outlet_free(x->posoutlet);
  outlet_free(x->sizeoutlet);
  outlet_free(x->extraoutlet);
}

static void *canvasposition_new(t_floatarg f)
{
  t_canvasposition *x = (t_canvasposition *)pd_new(canvasposition_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  int depth=(int)f;

  if(depth<0)depth=0;
  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_canvas = canvas;

  x->posoutlet=outlet_new(&x->x_obj, &s_list);
  x->sizeoutlet=outlet_new(&x->x_obj, &s_list);
  x->extraoutlet=outlet_new(&x->x_obj, 0);

  return (x);
}

void canvasposition_setup(void)
{
  iemguts_boilerplate("[canvasposition]", 0);
  canvasposition_class = class_new(gensym("canvasposition"),
                                   (t_newmethod)canvasposition_new, (t_method)canvasposition_free,
                                   sizeof(t_canvasposition), 0,
                                   A_DEFFLOAT, 0);
  class_addbang(canvasposition_class, (t_method)canvasposition_bang);
  class_addlist(canvasposition_class, (t_method)canvasposition_list);
#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION <= 0 && PD_MINOR_VERSION < 47)
  if(iemguts_check_atleast_pdversion(0,47,0)) {
    int got_major=0, got_minor=0, got_bugfix=0;
    sys_getversion(&got_major, &got_minor, &got_bugfix);
    error("[canvasposition] disabled zoom support at compile-time");
  }
#endif
}
