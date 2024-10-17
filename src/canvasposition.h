#ifndef canvasposition_h
#define canvasposition_h

#include "iemguts.h"
#include "g_canvas.h"

static t_float canvasposition_getzoom(const t_canvas*parent)
{
#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 47)
  if(parent && iemguts_check_atleast_pdversion(0,47,0)) {
    return parent->gl_zoom;
  }
#endif
  return 1.;
}


static void canvasposition_get(t_outlet*posout, t_outlet*sizeout, t_outlet*extraout,
                               t_canvas*parent, t_object*obj)
{
  t_float x0=0., y0=0., width=0., height=0.;
  int x1, y1, x2, y2;
  t_float zoom = canvasposition_getzoom(parent);
  t_atom alist[2];

  if(!obj) return;
  x1 = x2 = y1 = y2 = 0;

  if(parent) {
    width= (parent->gl_screenx2 - parent->gl_screenx1);
    height=(parent->gl_screeny2 - parent->gl_screeny1);

    gobj_getrect(&obj->te_g, parent, &x1, &y1, &x2, &y2);
  }
  x0=obj->te_xpix;
  y0=obj->te_ypix;


  if(parent) {
    SETFLOAT(alist+0, (x2-x1)/zoom);
    SETFLOAT(alist+1, (y2-y1)/zoom);
    outlet_anything(extraout, gensym("size"), 2, alist);
  }

  SETFLOAT(alist+0, zoom);
  outlet_anything(extraout, gensym("zoom"), 1, alist);

  if(parent) {
    SETFLOAT(alist+0, width);
    SETFLOAT(alist+1, height);
    outlet_list(sizeout, 0, 2, alist);
  }

  SETFLOAT(alist+0, x0);
  SETFLOAT(alist+1, y0);
  outlet_list(posout, 0, 2, alist);
}


static void canvasposition_set(t_canvas*parent, t_object*obj, t_float newX, t_float newY)
{
  t_float zoom = canvasposition_getzoom(parent);
  int dx = newX*zoom - obj->te_xpix;
  int dy = newY*zoom - obj->te_ypix;

  if ((0==dx)&&(0==dy))
    return; /* nothing to do */

  if(parent && glist_isvisible(parent))  {
    gobj_displace(&obj->te_g, parent, dx, dy);
    canvas_fixlinesfor(parent, obj);
  } else {
    obj->te_xpix+=dx;
    obj->te_ypix+=dy;
  }
}



#endif /* canvasposition_h */
