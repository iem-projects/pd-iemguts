#ifndef canvasposition_h
#define canvasposition_h

#include "iemguts.h"
#include "g_canvas.h"


static void canvasposition_get(t_outlet*posout, t_outlet*sizeout, t_outlet*extraout,
				t_canvas*c)
{
  t_canvas*c0=c?c->gl_owner:0;
  t_float x0=0., y0=0., width=0., height=0.;
  int x1, y1, x2, y2;
  t_float zoom = 1.;
  t_atom alist[2];

  if(!c) return;
  x1 = x2 = y1 = y2 = 0;

  if(c0) {
    width= (c0->gl_screenx2 - c0->gl_screenx1);
    height=(c0->gl_screeny2 - c0->gl_screeny1);

#if (defined PD_MAJOR_VERSION && defined PD_MINOR_VERSION) && (PD_MAJOR_VERSION > 0 || PD_MINOR_VERSION >= 47)
    if(iemguts_check_atleast_pdversion(0,47,0)) {
      zoom = c0->gl_zoom;
    }
#endif

    gobj_getrect(&c->gl_obj.te_g, c0, &x1, &y1, &x2, &y2);
  }
  x0=c->gl_obj.te_xpix;
  y0=c->gl_obj.te_ypix;


  if(c0) {
    SETFLOAT(alist+0, x2-x1);
    SETFLOAT(alist+1, y2-y1);
    outlet_anything(extraout, gensym("size"), 2, alist);
  }

  SETFLOAT(alist+0, zoom);
  outlet_anything(extraout, gensym("zoom"), 1, alist);

  if(c0) {
    SETFLOAT(alist+0, width);
    SETFLOAT(alist+1, height);
    outlet_list(sizeout, 0, 2, alist);
  }

  SETFLOAT(alist+0, x0/zoom);
  SETFLOAT(alist+1, y0/zoom);
  outlet_list(posout, 0, 2, alist);
}


#endif /* canvasposition_h */
