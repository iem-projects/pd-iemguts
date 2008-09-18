
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
#include <string.h>

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

typedef struct _intvec
{
  int num_elements; /* number of valid elements in the 'elements' vector */
  int*elements;     /* elements */
  int size;         /* private: the full length of the 'elements' vector */
} t_intvec;


static t_intvec*intvec_new(int initial_size)
{
  t_intvec*res=(t_intvec*)getbytes(sizeof(t_intvec));
  if(initial_size<1)
    initial_size=32;
  
  res->num_elements=0;
  res->size=initial_size;
  res->elements=(int*)getbytes(res->size*sizeof(int));

  return res;
}


static void intvec_free(t_intvec*vec)
{
  if(NULL==vec)return;
  if(vec->elements)
    freebytes(vec->elements, sizeof(int)*vec->size);
  vec->elements=NULL;

  vec->size=0;
  vec->num_elements=0;

  freebytes(vec, sizeof(t_intvec));
}


static t_intvec*intvec_add(t_intvec*vec, int element)
{
  /* ensure that vector is big enough */
  if(vec->size<=vec->num_elements) {
    /* resize! */
    t_intvec*vec2=intvec_new(2*vec->num_elements);
    memcpy(vec2->elements, vec->elements, vec->size);
    vec2->num_elements=vec->size;
    intvec_free(vec);
    vec=vec2;
  }

  /* add the new element to the end of the vector */
  vec->elements[vec->num_elements]=element;
  vec->num_elements++;

  return vec;
}

/* just for debugging... */
static void intvec_post(t_intvec*vec)
{
  int i=0;
  post("vec: 0x%X :: 0x%X holds %d/%d elements", vec, vec->elements, vec->num_elements, vec->size);
  startpost("elements:");
  for(i=0; i<vec->num_elements; i++) {
    startpost(" %02d", vec->elements[i]);
  }
  endpost();
}

static t_intvec**query_inletconnections(t_canvasconnections *x, int *num_inlets) {
  int i=0;
  t_intvec**invecs=NULL;
  int ninlets=0;
  int nin=0;
 
  t_gobj*y;

  if(0==x->x_object || 0==x->x_parent)
    return 0;

  ninlets=obj_ninlets(x->x_object);
  *num_inlets=ninlets;


    // TODO....find objects connecting TO this object
    /* as Pd does not have any information about connections to inlets, 
     * we have to find out ourselves
     * this is done by traversing all objects in the canvas and try
     * to find out, whether they are connected to us!
     */

  invecs=getbytes(sizeof(t_intvec*)*ninlets);
  for(i=0; i<ninlets; i++)invecs[i]=intvec_new(0);

  for (y = x->x_parent->gl_list; y; y = y->g_next) /* traverse all objects in canvas */
    {
      t_object*obj=(t_object*)y;
      int obj_nout=obj_noutlets(obj);
      int nout=0;
      for(nout=0; nout<obj_nout; nout++) { /* traverse all outlets of each object */
        t_outlet*out=0;
        t_inlet *in =0;
        t_object*dest=0;
        int sourcewhich=0;
        
        t_outconnect*conn=obj_starttraverseoutlet(obj, &out, nout);
        
        while(conn) { /* traverse all connections from each outlet */
          int which;
          conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
          if(dest==x->x_object) { // connected to us!
            int connid = glist_getindex(x->x_parent, (t_gobj*)obj);

            //            post("inlet from %d:%d to my:%d", connid, sourcewhich, which);

            /* add it to the inletconnectionlist */
            intvec_add(invecs[which], connid);
          }
          sourcewhich++;
        }
      }
    }
  return invecs;
}

static void canvasconnections_queryinlets(t_canvasconnections *x)
{
  t_atom at;
  int ninlets=0, i;
  t_intvec**invecs=query_inletconnections(x, &ninlets);

  SETFLOAT(&at, (t_float)ninlets);
  outlet_anything(x->x_out, gensym("inlets"), 1, &at);

  for(i=0; i<ninlets; i++){
    int size=invecs[i]->num_elements;
    t_atom*ap=getbytes(sizeof(t_atom)*(size+1));
    int j=0;
    SETFLOAT(ap, (t_float)i);
    for(j=0; j<size; j++)
      SETFLOAT(ap+j+1, ((t_float)invecs[i]->elements[j]));

    outlet_anything(x->x_out, gensym("inlet"), size+1, ap);

    intvec_free(invecs[i]);
    freebytes(ap, sizeof(t_atom)*(size+1));
  }
  if(invecs)freebytes(invecs, sizeof(t_intvec*)*ninlets);
}

static int canvasconnections_inlets(t_canvasconnections *x)
{
  t_atom at;
  int ninlets=0;

  if(0==x->x_object || 0==x->x_parent)
    return 0;

  ninlets=obj_ninlets(x->x_object);


  ninlets=obj_ninlets(x->x_object);
  SETFLOAT(&at, (t_float)ninlets);
  outlet_anything(x->x_out, gensym("inlets"), 1, &at);

  return ninlets;
}

static void canvasconnections_inlet(t_canvasconnections *x, t_floatarg f)
{
  int inlet=f;
  t_atom at[4];
  int ninlets=0;

  t_intvec**inlets=query_inletconnections(x, &ninlets);
  if(!inlets || inlet < 0 || inlet>ninlets) {
    /* non-existing inlet! */
    return;
  } else {
    post("TODO");

  }
}


static int canvasconnections_outlets(t_canvasconnections *x)
{
  t_atom at;
  int noutlets=0;

  if(0==x->x_object || 0==x->x_parent)
    return 0;

  noutlets=obj_noutlets(x->x_object);
  SETFLOAT(&at, (t_float)noutlets);
  outlet_anything(x->x_out, gensym("outlets"), 1, &at);

  return noutlets;
}

static void canvasconnections_outlet(t_canvasconnections *x, t_floatarg f)
{
  int outlet=f;
  t_atom at[4];
  int noutlets=0;

  if(0==x->x_object || 0==x->x_parent)
    return;

  noutlets=obj_noutlets(x->x_object);

  if(outlet<0 || outlet>=noutlets) {
    post("nonexisting outlet: %d", outlet);
    /* non-existing outlet! */
    return;
  } else {
    t_outlet*out=0;
    t_outconnect*conn=obj_starttraverseoutlet(x->x_object, &out, outlet);
    t_object*dest=0;
    t_inlet*in=0;
    int count=0;
    int id=glist_getindex(x->x_parent, (t_gobj*)x->x_object);

    conn=obj_starttraverseoutlet(x->x_object, &out, outlet);
    while(conn) {
      int destid=0;
      int destwhich=0;
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &destwhich);
      destid = glist_getindex(x->x_parent, (t_gobj*)dest);

      //post("connection from %d|%d to %d|%d", id, outlet, destid, destwhich);
      SETFLOAT(at+0, (t_float)id);
      SETFLOAT(at+1, (t_float)outlet);
      SETFLOAT(at+2, (t_float)destid);
      SETFLOAT(at+3, (t_float)destwhich);

      outlet_anything(x->x_out, gensym("outconnect"), 4, at);
    }
  }

}

static void canvasconnections_queryoutlets(t_canvasconnections *x)
{
  int noutlets=canvasconnections_outlets(x);
  int nout=0;
  t_atom at;
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
      int connid=0;
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
      connid = glist_getindex(x->x_parent, (t_gobj*)dest);

      SETFLOAT(abuf+1+i, (t_float)connid);
      i++;
    }
    outlet_anything(x->x_out, gensym("outlet"), count+1, abuf);
    freebytes(abuf, sizeof(t_atom)*(count+1));
  }
}

static void canvasconnections_bang(t_canvasconnections *x)
{
  canvasconnections_queryinlets(x);
  canvasconnections_queryoutlets(x);
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
  canvasconnections_class = class_new(gensym("canvasconnections"), 
                                      (t_newmethod)canvasconnections_new, (t_method)canvasconnections_free, 
                                      sizeof(t_canvasconnections), 0, 
                                      A_DEFFLOAT, 0);
  class_addbang(canvasconnections_class, (t_method)canvasconnections_bang);

  class_addmethod(canvasconnections_class, (t_method)canvasconnections_outlets, gensym("outlets"), 0);
  class_addmethod(canvasconnections_class, (t_method)canvasconnections_outlet, gensym("outlet"), A_FLOAT, 0);

  class_addmethod(canvasconnections_class, (t_method)canvasconnections_inlets, gensym("inlets"), 0);
  class_addmethod(canvasconnections_class, (t_method)canvasconnections_inlet, gensym("inlet"), A_FLOAT, 0);
}
