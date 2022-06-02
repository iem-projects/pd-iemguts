
/******************************************************
 *
 * canvasobjectconnections - implementation file
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
 * this object provides a way to send messages to query the connections
 * of the containing canvas;
 * but you can give the "depth" as argument;
 * e.g. [canvasobjectconnections 1] will query the parent of the containing canvas
 */

#include "iemguts.h"
#include "g_canvas.h"
#include "m_imp.h"
#include <string.h>

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- canvasobjectconnections ---------------------------- */

static t_class *canvasobjectconnections_class;

typedef struct _canvasobjectconnections
{
  t_object  x_obj;
  t_canvas  *x_parent;
  int        x_index;
  t_outlet  *x_out;
} t_canvasobjectconnections;

typedef struct _intvec
{
  int num_elements; /* number of valid elements in the 'elements' vector */
  int*elements;     /* elements */
  int size;         /* private: the full length of the 'elements' vector */
} t_intvec;


static t_intvec*intvec_new(int initial_size)
{
  t_intvec*res=getbytes(sizeof(*res));
  if(initial_size<1)
    initial_size=32;

  res->num_elements=0;
  res->size=initial_size;
  res->elements=getbytes(res->size*sizeof(*res->elements));

  return res;
}


static void intvec_free(t_intvec*vec)
{
  if(NULL==vec)return;
  if(vec->elements)
    freebytes(vec->elements, vec->size * sizeof(*vec->elements));
  vec->elements=NULL;

  vec->size=0;
  vec->num_elements=0;

  freebytes(vec, sizeof(*vec));
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
  return pd_checkobject(&gobj->g_pd);
}

static int query_inletconnections(t_canvas*parent, t_object*object,
                                  t_intvec***outobj, t_intvec***outwhich) {
  int i=0;
  t_intvec**invecs=NULL;
  t_intvec**inwhich=NULL;
  int ninlets=0;

  t_gobj*y;

  if(0==object || 0==parent)
    return 0;

  ninlets=obj_ninlets(object);

    // TODO....find objects connecting TO this object
    /* as Pd does not have any information about connections to inlets,
     * we have to find out ourselves
     * this is done by traversing all objects in the canvas and try
     * to find out, whether they are connected to us!
     */

  invecs =getbytes(ninlets*sizeof(*invecs));
  inwhich=getbytes(ninlets*sizeof(*inwhich));
  for(i=0; i<ninlets; i++){
    invecs[i]=intvec_new(0);
    inwhich[i]=intvec_new(0);
  }

  for (y = parent->gl_list; y; y = y->g_next) /* traverse all objects in canvas */
    {
      t_object*obj=(t_object*)y;
      int obj_nout=obj_noutlets(obj);
      int nout=0;
      int sourcewhich=0;

      for(nout=0; nout<obj_nout; nout++) { /* traverse all outlets of each object */
        t_outlet*out=0;
        t_inlet *in =0;
        t_object*dest=0;

        t_outconnect*conn=obj_starttraverseoutlet(obj, &out, nout);

        while(conn) { /* traverse all connections from each outlet */
          int which;
          conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
          if(dest==object) { // connected to us!
            int connid = glist_getindex(parent, (t_gobj*)obj);

            //            post("inlet from %d:%d to my:%d", connid, sourcewhich, which);

            /* add it to the inletconnectionlist */
            intvec_add(invecs[which], connid);
            intvec_add(inwhich[which], sourcewhich);
          }
        }
        sourcewhich++;

      }
    }
  if(outobj)*outobj=invecs;
  if(outwhich)*outwhich=inwhich;

  // return invecs;
  return ninlets;
}


static void canvasobjectconnections_index(t_canvasobjectconnections *x, t_float findex)
{
  int index=findex;
  if(index2obj(x->x_parent, index))
    x->x_index = index;
  else
    pd_error(x, "object index %d out of range", index);
}

static void canvasobjectconnections_queryinlets(t_canvasobjectconnections *x)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  t_atom at;
  t_intvec**invecs=0;
  int ninlets=query_inletconnections(x->x_parent, object, &invecs, 0);
  int i;

  SETFLOAT(&at, (t_float)ninlets);
  outlet_anything(x->x_out, gensym("inlets"), 1, &at);

  for(i=0; i<ninlets; i++){
    int size=invecs[i]->num_elements;
    if(size>0) {
      t_atom*ap=getbytes((size+1)*sizeof(*ap));
      int j=0;
      t_symbol*s=gensym("inlet");

      SETFLOAT(ap, (t_float)i);
      for(j=0; j<size; j++)
        SETFLOAT(ap+j+1, ((t_float)invecs[i]->elements[j]));

      outlet_anything(x->x_out, s, size+1, ap);
      freebytes(ap, (size+1) * sizeof(*ap));
    }
    intvec_free(invecs[i]);
  }
  if(invecs)freebytes(invecs, ninlets * sizeof(*invecs));
}

static void canvasobjectconnections_inlet(t_canvasobjectconnections *x, t_floatarg f)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  int inlet=f;
  t_intvec**invecs=0;
  int ninlets=query_inletconnections(x->x_parent, object, &invecs, 0);

  if(inlet >= 0 && inlet < ninlets) {
    int size=invecs[inlet]->num_elements;
    t_atom*ap=getbytes((size+1)*sizeof(*ap));
    t_symbol*s=gensym("inlet");
    if(obj_issignalinlet(object,inlet)) {
      s=gensym("inlet~");
    }

    SETFLOAT(ap, (t_float)inlet);

    if(size>0) {
      int j=0;
      for(j=0; j<size; j++)
        SETFLOAT(ap+j+1, ((t_float)invecs[inlet]->elements[j]));
    }

    outlet_anything(x->x_out, s, size+1, ap);
    freebytes(ap, (size+1) * sizeof(*ap));

    intvec_free(invecs[inlet]);
  }
  if(invecs)freebytes(invecs, ninlets * sizeof(*invecs));
}



static int canvasobjectconnections_inlets(t_canvasobjectconnections *x)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  t_atom at;
  int ninlets=0;

  if(0==object || 0==x->x_parent)
    return 0;

  ninlets=obj_ninlets(object);
  //  ninlets=obj_ninlets(object);
  SETFLOAT(&at, (t_float)ninlets);
  outlet_anything(x->x_out, gensym("inlets"), 1, &at);

  return ninlets;
}

static void canvasobjectconnections_inconnect(t_canvasobjectconnections *x, t_floatarg f)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  const int inlet=f;
  t_intvec**invecs=0;
  t_intvec**inwhich=0;
  int ninlets=query_inletconnections(x->x_parent, object, &invecs, &inwhich);

  if(!ninlets || inlet < 0 || inlet>ninlets) {
    post("nonexisting inlet: %d", inlet);
    /* non-existing inlet! */
    return;
  } else {
    int i;
    t_atom at[4];
    int id=glist_getindex(x->x_parent, (t_gobj*)object);

    for(i=0; i<ninlets; i++){
      if(inlet==i) {
        int j=0;
        for(j=0; j<invecs[i]->num_elements; j++) {
          SETFLOAT(at+0, (t_float)invecs[i]->elements[j]);
          SETFLOAT(at+1, (t_float)inwhich[i]->elements[j]);
          SETFLOAT(at+2, (t_float)id);
          SETFLOAT(at+3, (t_float)i);
          outlet_anything(x->x_out, gensym("inconnect"), 4, at);
        }
      }
      intvec_free(invecs[i]);
      intvec_free(inwhich[i]);
    }
  }
  if(invecs) freebytes(invecs , ninlets * sizeof(*invecs));
  if(inwhich)freebytes(inwhich, ninlets * sizeof(*inwhich));
}


static int canvasobjectconnections_outlets(t_canvasobjectconnections *x)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  t_atom at;
  int noutlets=0;

  if(0==object || 0==x->x_parent)
    return 0;

  noutlets=obj_noutlets(object);
  SETFLOAT(&at, (t_float)noutlets);
  outlet_anything(x->x_out, gensym("outlets"), 1, &at);

  return noutlets;
}

static void canvasobjectconnections_outconnect(t_canvasobjectconnections *x, t_floatarg f)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  int outlet=f;
  t_atom at[4];
  int noutlets=0;

  if(0==object || 0==x->x_parent)
    return;

  noutlets=obj_noutlets(object);

  if(outlet<0 || outlet>=noutlets) {
    post("nonexisting outlet: %d", outlet);
    /* non-existing outlet! */
    return;
  } else {
    t_outlet*out=0;
    t_outconnect*conn=obj_starttraverseoutlet(object, &out, outlet);
    t_object*dest=0;
    t_inlet*in=0;
    int id=glist_getindex(x->x_parent, (t_gobj*)object);

    conn=obj_starttraverseoutlet(object, &out, outlet);
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

static void canvasobjectconnections_outlet(t_canvasobjectconnections *x, t_floatarg f)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  int outlet=f;
  int noutlets=0;

  if(0==object || 0==x->x_parent)
    return;

  noutlets=obj_noutlets(object);

  if(outlet >= 0 && outlet < noutlets) {
    t_outlet*out=0;
    t_inlet*in=0;
    t_object*dest=0;
    int which;
    t_outconnect*conn=obj_starttraverseoutlet(object, &out, outlet);
    t_atom*abuf=0;
    int count=0;

    t_symbol*s=gensym("outlet");
    if(obj_issignaloutlet(object,outlet)) {
      s=gensym("outlet~");
    }

    while(conn) {
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
      count++;
    }
    abuf=getbytes((count+1)*sizeof(*abuf));
    SETFLOAT(abuf, outlet);

    if(count>0) {
      int i=0;
      conn=obj_starttraverseoutlet(object, &out, outlet);
      while(conn) {
        int connid=0;
        conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
        connid = glist_getindex(x->x_parent, (t_gobj*)dest);

        SETFLOAT(abuf+1+i, (t_float)connid);
        i++;
      }
    }
    outlet_anything(x->x_out, s, count+1, abuf);
    freebytes(abuf, (count+1) * sizeof(*abuf));
  }
}

static void canvasobjectconnections_queryoutlets(t_canvasobjectconnections *x)
{
  t_object*object = index2obj(x->x_parent, x->x_index);
  int noutlets=canvasobjectconnections_outlets(x);
  int nout=0;
  t_atom at;

  SETFLOAT(&at, (t_float)noutlets);
  outlet_anything(x->x_out, gensym("outlets"), 1, &at);

  for(nout=0; nout<noutlets; nout++) {
    t_outlet*out=0;
    t_outconnect*conn=obj_starttraverseoutlet(object, &out, nout);
    t_object*dest=0;
    t_inlet*in=0;
    int which=0;
    int count=0;
    while(conn) {
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
      count++;
    }
    if(count>0) {
      int i=0;
      t_atom*abuf=getbytes((count+1)*sizeof(*abuf));
      SETFLOAT(abuf, nout);
      conn=obj_starttraverseoutlet(object, &out, nout);
      while(conn) {
        int connid=0;
        conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
        connid = glist_getindex(x->x_parent, (t_gobj*)dest);

        SETFLOAT(abuf+1+i, (t_float)connid);
        i++;
      }
      outlet_anything(x->x_out, gensym("outlet"), count+1, abuf);
      freebytes(abuf, (count+1) * sizeof(*abuf));
    }
  }
}

static void canvasobjectconnections_bang(t_canvasobjectconnections *x)
{
  canvasobjectconnections_queryinlets(x);
  canvasobjectconnections_queryoutlets(x);
}


static void canvasobjectconnections_free(t_canvasobjectconnections *x)
{
  outlet_free(x->x_out);
  x->x_out=0;
}

static void *canvasobjectconnections_donew(int depth, int index)
{
  t_canvasobjectconnections *x = (t_canvasobjectconnections *)pd_new(canvasobjectconnections_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);

  x->x_parent=0;

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_parent = canvas->gl_owner;
  x->x_index = index;

  x->x_out=outlet_new(&x->x_obj, 0);

  return (x);
}

static void *canvasobjectconnections_new(t_symbol*s, int argc, t_atom*argv)
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
    return canvasobjectconnections_donew(depth, index);
  }
}


void canvasobjectconnections_setup(void)
{
  iemguts_boilerplate("[canvasobjectconnections]", 0);
  canvasobjectconnections_class = class_new(gensym("canvasobjectconnections"),
                                            (t_newmethod)canvasobjectconnections_new,
                                            (t_method)canvasobjectconnections_free,
                                            sizeof(t_canvasobjectconnections), 0,
                                            A_GIMME, 0);
  class_addbang(canvasobjectconnections_class, (t_method)canvasobjectconnections_bang);
  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_index, gensym("object"), A_FLOAT, 0);

  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_outlets, gensym("outlets"), 0);
  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_outlet, gensym("outlet"), A_FLOAT, 0);
  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_outconnect, gensym("outconnect"), A_FLOAT, 0);

  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_inlets, gensym("inlets"), 0);
  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_inlet, gensym("inlet"), A_FLOAT, 0);
  class_addmethod(canvasobjectconnections_class, (t_method)canvasobjectconnections_inconnect, gensym("inconnect"), A_FLOAT, 0);
}
