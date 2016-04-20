/******************************************************
 *
 * patcherize - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2016:forum::für::umläute:2016
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/


/*
 *
 *  - patcherize selection
 *
 * send a 'patcherize' message to the canvas, and all currently selected objects will
 *  e moved into a newly created subpatch
 *
 * TODO:
 * - inlets/outlets
 *   + inlets: any outlet that is connected TO one of the patcherized objects, is turned into an [inlet]
 *             (so we don't need to worry about connections mixing signal/msg)
 *   + outlets: any outlet that is going out of the patcherized is connected to an [outlet]
 * - position
 *   + move patcherized objects to the top/left corner
 *   + make subpatch window large enough to hold the entire patcherized content
 * - subpatch label
 *   + if the containing canvas is visible, go into edit-mode and let the user type a canvas-name immediately
 *
 * LATER:
 *  - save selection to file
 *
 */

#include "iemguts.h"

#include "g_canvas.h"
#include "m_imp.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>
/* ------------------------- patcherize ---------------------------- */

static void print_glist(t_glist*glist) {
  t_gobj*obj = NULL;
  if(NULL == glist)return;
  post("\tglist=%p", glist);
  for(obj=glist->gl_list; obj; obj=obj->g_next) { post ("\t%p [%p]", obj, obj->g_next); }
}
static int glist_suspend_editor(t_glist*glist) {
  int wanteditor = (NULL != glist->gl_editor);
  canvas_destroy_editor(glist);
  return wanteditor;
}

static void glist_resume_editor(t_glist*glist, int wanteditor) {
  if(wanteditor) {
    canvas_create_editor(glist);
  }
  glist_redraw(glist);
}


struct _patcherize_connectto
{
  t_object *object;   /* object that we connect to */
  unsigned int index; /* which inlet of the given object is this? */
  struct _patcherize_connectto *next;
};

/* connection between subpatch and surrounding environment */
typedef struct _patcherize_connection {
  int is_signal;      /* is this a signal outlet? */
  t_object*object;    /* object that we connect from */
  unsigned int index; /* which outlet of the given object is this? */
  struct _patcherize_connectto*to; /* list of connections */
  struct _patcherize_connection*next;
} t_patcherize_connection;
typedef struct _patcherize_connections {
  t_patcherize_connection*inlets;
  t_patcherize_connection*outlets;
} t_patcherize_connections;
static void insert_connection_to(t_patcherize_connection*iolets,t_object*to_obj, unsigned int to_index) {
  struct _patcherize_connectto*dest=iolets->to, *last=NULL;
  while(dest) {
    if((dest->object == to_obj) && (dest->index == to_index)) /* already inserted */
      return;
    last=dest;
    dest=dest->next;
  }
  dest=calloc(1, sizeof(*dest));
  dest->object=to_obj;
  dest->index=to_index;
  if(last)
    last->next=dest;
  else
    iolets->to=dest;
}
static t_patcherize_connection*create_connection(t_object*from_obj, int from_index, t_object*to_obj, int to_index) {
  t_patcherize_connection*conn=(t_patcherize_connection*)calloc(1, sizeof(*conn));
  conn->is_signal=obj_issignaloutlet(from_obj, from_index);
  conn->object=from_obj;
  conn->index=from_index;
  insert_connection_to(conn, to_obj, to_index);
  return conn;
}
static t_patcherize_connection*insert_connection(t_patcherize_connection*iolets,
						 t_object*from_obj, unsigned int from_index,
						 t_object*to_obj, unsigned int to_index) {
  /* check whether iolets already contains from_obj/from_index */
  t_patcherize_connection*cur=iolets, *last=NULL;
  if(!cur) {
    return create_connection(from_obj, from_index, to_obj, to_index);
  }
  while(cur) {
    if((cur->object == from_obj) && (cur->index == from_index)) {
      /* found it; insert new 'to' */
      insert_connection_to(cur, to_obj, to_index);
      return iolets;
    }
    last=cur;
    cur=cur->next;
  }
  /* if we reach this, then we didn't find the output in our list; so create it */
  /* LATER: insert the new connection at a sorted location */
  last->next=create_connection(from_obj, from_index, to_obj, to_index);
  return iolets;
}
static void print_conns(const char*name, t_patcherize_connection*conn) {
  post("%s: %p", name, conn);
  while(conn) {
    struct _patcherize_connectto *to=conn->to;
    while(to) {
      post("%s%p[%d] -> %p[%d]", name, conn->object, conn->index, to->object, to->index);
      to=to->next;
    }
    conn=conn->next;
  }
}
static t_patcherize_connection*get_object_connections(t_patcherize_connection*iolets, t_glist*cnv, t_object*obj) {
  int sel=glist_isselected(cnv,&obj->te_g);
  int obj_nout=obj_noutlets(obj);
  int nout=0;
  for(nout=0; nout<obj_nout; nout++) { /* traverse all outlets of the object */
    t_outlet*out=0;
    t_outconnect*conn=obj_starttraverseoutlet(obj, &out, nout);
    while(conn) {
      int which;
      t_object*dest=0;
      t_inlet *in =0;
      conn=obj_nexttraverseoutlet(conn, &dest, &in, &which);
      if (glist_isselected(cnv, &(dest->te_g)) != sel) {
	/* this is a connection crossing the selection boundary; insert it */
	iolets=insert_connection(iolets, obj, nout, dest, which);
      }
    }
  }
  return iolets;
}

static t_patcherize_connections*get_connections(t_glist*cnv) {
  t_patcherize_connections*connections=(t_patcherize_connections*)calloc(1, sizeof(*connections));
  /* 1. iterate over all the objects in the canvas, and store any connecting objects */
  t_gobj*gobj=NULL;
  for(gobj=cnv->gl_list; gobj; gobj=gobj->g_next) {
    t_object*obj=pd_checkobject(&gobj->g_pd);
    if(!obj)continue;
    if(glist_isselected(cnv, gobj)) {
      connections->outlets=get_object_connections(connections->outlets, cnv, obj);
    } else {
      connections->inlets=get_object_connections(connections->inlets, cnv, obj);
    }
  }
  return connections;
}
static void free_connectto(struct _patcherize_connectto*conn) {
  struct _patcherize_connectto*next=0;
  while(conn) {
    next=conn->next;
    conn->object=NULL;
    conn->index=0;
    conn->next=NULL;
    free(conn);
    conn=next;
  }
}
static void free_connection(t_patcherize_connection*conn) {
  t_patcherize_connection*next=0;
  while(conn) {
    next=conn->next;
    free_connectto(conn->to);
    conn->object=NULL;
    conn->index=0;
    conn->to=NULL;
    conn->next=NULL;
    free(conn);
    conn=next;
  }
}
static void free_connections(t_patcherize_connections*conns) {
  free_connection(conns->inlets);
  free_connection(conns->outlets);
  conns->inlets=conns->outlets=NULL;
  free(conns);
}

static void patcherize_boundary_disconnect(t_patcherize_connection*from) {
  while(from) {
    struct _patcherize_connectto*to=from->to;
    while(to) {
      obj_disconnect(from->object, from->index, to->object, to->index);
      to=to->next;
    }
    from=from->next;
  }
}
static void patcherize_boundary_reconnect(t_canvas*cnv,t_patcherize_connections*connections) {
  unsigned int index=0;
  t_gobj*gobj=cnv->gl_list;
  t_patcherize_connection*conns=connections->inlets;
  while(conns) {
    struct _patcherize_connectto*to=conns->to;
    /* connect outside objects with new subpatch */
    obj_connect(conns->object, conns->index, cnv, index);
    while(to) {
      /* connect [inlet]s with inside objects */
      obj_connect(gobj, 0, to->object, to->index);
      to=to->next;
    }
    index++;
    gobj=gobj->g_next;
    conns=conns->next;
  }
  conns=connections->outlets;
  index=0;
  while(conns) {
    struct _patcherize_connectto*to=conns->to;
    /* connect inside objects with [outlet]s */
    obj_connect(conns->object, conns->index, gobj, 0);
    while(to) {
      /* connect subpatch with outside objects */
      obj_connect(cnv, index, to->object, to->index);
      to=to->next;
    }
    index++;
    conns=conns->next;
    gobj=gobj->g_next;
  }
}

static void patcherize_fixcoordinates(unsigned int argc, t_gobj**argv, int xmin, int ymin) {
  unsigned int i;
  //post("fixing coords to %d/%d", xmin, ymin);
  for(i=0; i<argc; i++) {
    t_object*obj=pd_checkobject(&argv[i]->g_pd);
    if(obj) {
      int x = obj->te_xpix;
      int y = obj->te_ypix;
      //post("X: %d -> %d", x, x - xmin + 30);
      //post("Y: %d -> %d", y, y - ymin + 30);
      obj->te_xpix = x - xmin + 30;
      obj->te_ypix = y - ymin + 60;
    }
  }

}

static t_glist*patcherize_makesub(t_canvas*cnv, const char* name,
				  int X, int Y,
				  int xmin, int ymin, int xmax, int ymax,
				  int xwin, int ywin,
				  t_patcherize_connections*connections) {
  t_binbuf*b=NULL;
  t_gobj*result=NULL;
  t_patcherize_connection*iolets=NULL;
  int x, y;

  /* save and clear bindings to symbols #a, $N, $X; restore when done */
  t_pd *boundx = s__X.s_thing, *boundn = s__N.s_thing;
  s__X.s_thing = &cnv->gl_pd;
  s__N.s_thing = &pd_canvasmaker;

  int width=xmax-xmin;
  int height=ymax-ymin;
  if(width<200) width=200;
  if(height<100)height=100;

  post("boundingbox: %d/%d || %d/%d", xmin, ymin, xmax, ymax);

  b=binbuf_new();
  binbuf_addv(b, "ssiiiisi;", gensym("#N"), gensym("canvas"), xwin+xmin, ywin+ymin, width, height, gensym(name), 0);

  iolets=connections->inlets;
  x=20;  y=20;
  while(iolets) {
    binbuf_addv(b, "ssiis;", gensym("#X"), gensym("obj"), x, y,
		obj_issignaloutlet(iolets->object, iolets->index)?gensym("inlet~"):gensym("inlet"));
    x+=50;
    iolets=iolets->next;
  }
  iolets=connections->outlets;
  x=20; y=height-30;
  while(iolets) {
    binbuf_addv(b, "ssiis;", gensym("#X"), gensym("obj"), x, y,
		obj_issignaloutlet(iolets->object, iolets->index)?gensym("outlet~"):gensym("outlet"));
    x+=50;
    iolets=iolets->next;
  }
  binbuf_addv(b, "ssiiss;", gensym("#X"), gensym("restore"), X, Y, gensym("pd"), gensym(name));

  binbuf_print(b);

  binbuf_eval(b, 0,0,0);
  binbuf_free(b);

  s__X.s_thing = boundx;
  s__N.s_thing = boundn;
  for(result=cnv->gl_list; result->g_next;) result=result->g_next;
  return result;
}

static void canvas_patcherize(t_glist*cnv) {
  /* migrate selected objects from one canvas to another without re-instantiating them */
  int dspstate = 0;
  int editFrom = 0;
  t_gobj*gobj = NULL, *last=NULL;
  int objcount=0;
  t_gobj**gobjs=0;
  t_glist*to;
  int i=0;
  int xpos=0, ypos=0;
  int xmin, ymin, xmax, ymax;
  int numins=0, numouts=0;
  t_patcherize_connections*connections;
  if(NULL == cnv)return;
  xmin=ymin=INT_MAX;
  xmax=ymax=INT_MIN;

  /* store all the selected objects.
   * this needs to be done because the GUI-cleanup in glist_suspend_editor()
   * will undo any selection...
   */
  gobjs=getbytes(0*sizeof(*gobjs));
  for(gobj=cnv->gl_list; gobj; gobj=gobj->g_next) {
    if(glist_isselected(cnv, gobj)) {
      t_object*obj=pd_checkobject(&gobj->g_pd);
      if(obj) {
	int x=obj->te_xpix;
	int y=obj->te_ypix;
	xpos+=x;
	ypos+=y;
	if(xmin>x)xmin=x;if(xmax<x)xmax=x;
	if(ymin>y)ymin=y;if(ymax<y)ymax=y;
      }
      gobjs=resizebytes(gobjs, (objcount)*sizeof(*gobjs), (objcount+1)*sizeof(*gobjs));
      gobjs[objcount]=gobj;
      objcount++;
    }
  }


  /* if nothing is selected, we are done... */
  if(!objcount) {
    freebytes(gobjs,objcount*sizeof(*gobjs));
    return;
  }

  connections=get_connections(cnv);
  t_patcherize_connection*iolets=connections->inlets;

  numins=0;
  while(iolets) {
    iolets=iolets->next;
    numins++;
  }

  iolets=connections->outlets;
  numouts=0;
  while(iolets) {
    iolets=iolets->next;
    numouts++;
  }
  dspstate=canvas_suspend_dsp();

  /* disconnect the boundary connections */
  patcherize_boundary_disconnect(connections->inlets);
  patcherize_boundary_disconnect(connections->outlets);

  /* create a new sub-patch to pacherize into */
  to=patcherize_makesub(cnv, "*patcherized*", xpos/objcount, ypos/objcount,
			xmin, ymin, xmax+50, ymax+150,
			cnv->gl_screenx1,cnv->gl_screeny1,
			connections);

  editFrom=glist_suspend_editor(cnv);

  /* move the objects to the new subcanvas */
  for(i=0; i<objcount; i++) {
    t_gobj*gobj2 = NULL;
    int doit=0;
    gobj=gobjs[i];

    /* find the gobj that points to the current one (stored in 'last') */
    for(gobj2=cnv->gl_list; gobj2; last=gobj2, gobj2=gobj2->g_next) {
      if (gobj == gobj2) {
	doit=1;
	break;
      }
    }
    if (!doit)continue;

    /* remove the object from the 'from'-canvas */
    if (last)
      last->g_next = gobj->g_next;
    else
      cnv->gl_list = gobj->g_next;

    /* append it to the 'to'-canvas */
    if(to->gl_list) {
      for(gobj2=to->gl_list; gobj2 && gobj2->g_next;) gobj2=gobj2->g_next;
      gobj2->g_next = gobj;
    } else {
      to->gl_list = gobj;
    }
    gobj->g_next = 0;
  }

  patcherize_fixcoordinates(objcount, gobjs, xmin, ymin);

  /* reconnect the boundary connections */
  //print_conns("inlets :",connections->inlets);
  //print_conns("outlets:",connections->outlets);
  patcherize_boundary_reconnect(to, connections);
  glist_resume_editor(cnv, editFrom);
  canvas_redraw(cnv);
  canvas_resume_dsp(dspstate);
}

void patcherize_setup(void)
{
  if(NULL==canvas_class)return;
  iemguts_boilerplate("patcherize - turn objects into a subpatch", 0);

  if(NULL==zgetfn(&canvas_class, gensym("patcherize")))
    class_addmethod(canvas_class, (t_method)canvas_patcherize, gensym("patcherize"), 0);
}
