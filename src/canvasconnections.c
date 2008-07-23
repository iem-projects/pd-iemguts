
/******************************************************
 *
 * canvasconnections - implementation file
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
 * this object provides a way to send messages to upstream canvases
 * by default it sends messages to the containing canvas, but you can give the
 * "depth" as argument;
 * e.g. [canvasconnections 1] will send messages to the parent of the containing canvas
 */

#include "m_pd.h"
#include "g_canvas.h"
#include "m_imp.h"

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- canvasconnections ---------------------------- */

static t_class *canvasconnections_class;

typedef struct _canvasconnections
{
  t_object  x_obj;
  t_canvas  *x_parent;
  t_object  *x_object;
  t_outlet  *x_out;
} t_canvasconnections;

static void canvasconnections_outlets(t_canvasconnections *x)
{
  int noutlets=0;
  int nout=0;
  t_atom at;

  if(0==x->x_object || 0==x->x_parent)
    return;

  noutlets=obj_noutlets(x->x_object);
  SETFLOAT(&at, (t_float)noutlets);

  outlet_anything(x->x_out, gensym("outlets"), 1, &at);
  for(nout=0; nout<noutlets; nout++) {
    t_outlet*out=0;
    t_outconnect*conn=obj_starttraverseoutlet(x->x_object, &out, nout);
    t_object*dest=0;
    t_inlet*in=0;
    int which=0;
    int count=0, i=0;
    t_atom*abuf=0;
    while(conn) {
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
      count++;
    }
    abuf=(t_atom*)getbytes(sizeof(t_atom)*(count+1));
    SETFLOAT(abuf, nout);
    conn=obj_starttraverseoutlet(x->x_object, &out, nout);
    while(conn) {
      t_float connid=0;
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
      connid = glist_getindex(x->x_parent, (t_gobj*)dest);
      SETFLOAT(abuf+1+i, connid);
      i++;
    }
    outlet_anything(x->x_out, gensym("outlet"), count+1, abuf);
    freebytes(abuf, sizeof(t_atom)*(count+1));
  }
}

static void canvasconnections_bang(t_canvasconnections *x)
{
  canvasconnections_outlets(x);
}


static void canvasconnections_free(t_canvasconnections *x)
{
  x->x_object=0;
  outlet_free(x->x_out);
  x->x_out=0;
}

static void *canvasconnections_new(t_floatarg f)
{
  t_canvasconnections *x = (t_canvasconnections *)pd_new(canvasconnections_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  int depth=(int)f;
  if(depth<0)depth=0;

  x->x_parent=0;
  x->x_object=0;

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }


  if(canvas) {
    x->x_object = pd_checkobject((t_pd*)canvas);
    x->x_parent = canvas->gl_owner;
  }

  x->x_out=outlet_new(&x->x_obj, 0);

  return (x);
}

void canvasconnections_setup(void)
{
  canvasconnections_class = class_new(gensym("canvasconnections"), (t_newmethod)canvasconnections_new,
                               (t_method)canvasconnections_free, sizeof(t_canvasconnections), 0, A_DEFFLOAT, 0);
  class_addbang(canvasconnections_class, (t_method)canvasconnections_bang);
}
