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
 *  be moved into a newly created subpatch
 *
 * TODO:
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

/* returns 0 if the object is to be excluded from patcherization */
static t_symbol**s_excluded_classnames = 0;
static void build_exclude_list(void) {
  int i=0;
  s_excluded_classnames=getbytes(5 * sizeof(*s_excluded_classnames));

  s_excluded_classnames[i++]=gensym("inlet");
  s_excluded_classnames[i++]=gensym("outlet");
  s_excluded_classnames[i++]=gensym("inlet~");
  s_excluded_classnames[i++]=gensym("outlet~");
  s_excluded_classnames[i++]=0;
}
static int include_in_patcherization(const t_object*obj) {
  const t_symbol*c_name=obj->te_g.g_pd->c_name;
  t_symbol**excluded;
  if(!s_excluded_classnames)build_exclude_list();
  for(excluded=s_excluded_classnames;*excluded; excluded++) {
    if(c_name == *excluded)return 0;
  }

  return 1;
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
static int patcherize_conn_leftof_ref(t_object*ref_obj, unsigned int ref_idx, t_object*obj, unsigned int idx) {
  if(obj->te_xpix < ref_obj->te_xpix) return 1;
  if(obj->te_xpix > ref_obj->te_xpix) return 0;
  if(idx < ref_idx) return 1;
  if(idx > ref_idx) return 0;
  if(obj->te_ypix < ref_obj->te_ypix) return 1;
  if(obj->te_ypix > ref_obj->te_ypix) return 0;
  return 0;
}
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
  t_patcherize_connection*conn=calloc(1, sizeof(*conn));
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
  t_patcherize_connection*cur=iolets, *last=NULL, *conn=NULL;
  if(!cur) {
    return create_connection(from_obj, from_index, to_obj, to_index);
  }
  while(cur) {
    if((cur->object == from_obj) && (cur->index == from_index)) {
      /* found it; insert new 'to' */
      insert_connection_to(cur, to_obj, to_index);
      return iolets;
    }
    cur=cur->next;
  }
  /* if we reach this, then we didn't find the output in our list; so create it */
  /* LATER: insert the new connection at a sorted location */
  conn=create_connection(from_obj, from_index, to_obj, to_index);

  /* inserted into sorted list */
  cur=iolets; last=NULL;
  while(cur) {
    if(patcherize_conn_leftof_ref(cur->object, cur->index, from_obj, from_index)) {
      /* we sort before current element, so insert! */
      conn->next=cur;
      if(last) {
	last->next=conn;
      } else {
	/* insert beginning */
	iolets=conn;
      }
      return iolets;
    }
    last=cur;
    cur=cur->next;
  }
  /* if we reached this, then the new outlet sorts last */
  last->next=conn;
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
  t_patcherize_connections*connections=calloc(1, sizeof(*connections));
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
static unsigned int get_maxdollarg(t_object*obj) {
  t_binbuf*b=obj->te_binbuf;
  int argc=binbuf_getnatom(b);
  t_atom*argv=binbuf_getvec(b);
  unsigned int result=0;
  const t_symbol*c_name=obj->te_g.g_pd->c_name;
  if(c_name == gensym("canvas") && !canvas_isabstraction(obj)) {
    // a subpatch! recurse
    t_gobj*gobj;
    for(gobj=((t_glist*)obj)->gl_list; gobj; gobj=gobj->g_next) {
      unsigned int subdollar=get_maxdollarg(gobj);
      if(subdollar>result)result=subdollar;
    }
  }

  while(argc--) {
    t_atom*a=argv++;
    int dollarg=0;
    if(A_DOLLAR == a->a_type)
      dollarg=a->a_w.w_index;
    else if(A_DOLLSYM == a->a_type) {
      const char*s=a->a_w.w_symbol->s_name;
      while(*s){
	if('$' == *s) {
	  const char*endptr=0;
	  long int v=strtol(s+1, &endptr, 10);
	  s=endptr;
	  if(v>dollarg)dollarg=v;
	} else
	  s++;
      }
    }
    if(dollarg>result)result=dollarg;
  }
  return result;
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
    obj_connect(conns->object, conns->index, (t_object*)cnv, index);
    while(to) {
      /* connect [inlet]s with inside objects */
      obj_connect((t_object*)gobj, 0, to->object, to->index);
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
    obj_connect(conns->object, conns->index, (t_object*)gobj, 0);
    while(to) {
      /* connect subpatch with outside objects */
      obj_connect((t_object*)cnv, index, to->object, to->index);
      to=to->next;
    }
    index++;
    conns=conns->next;
    gobj=gobj->g_next;
  }
}

static void patcherize_fixcoordinates(unsigned int argc, t_gobj**argv, int xmin, int ymin) {
  unsigned int i;
  for(i=0; i<argc; i++) {
    t_object*obj=pd_checkobject(&argv[i]->g_pd);
    if(obj) {
      int x = obj->te_xpix;
      int y = obj->te_ypix;
      obj->te_xpix = x - xmin + 30;
      obj->te_ypix = y - ymin + 60;
    }
  }

}

static t_glist*patcherize_makesub(t_canvas*cnv,
				  const char* name, /* subpatch name of filename */
				  int*save2file_,
				  int X, int Y,
				  int xmin, int ymin, int xmax, int ymax,
				  int xwin, int ywin,
				  t_patcherize_connections*connections,
				  unsigned int maxdollarg);
static t_glist*
patcherize_makesub_tryagain(t_canvas*cnv,
			   const char*name, int*save2file,
			   int X, int Y,
			   int xmin, int ymin, int xmax, int ymax,
			   int xwin, int ywin,
			   t_patcherize_connections*connections,
			   unsigned int maxdollarg,
			   t_binbuf*b, t_pd *boundx, t_pd *boundn) {
  /* things went wrong, try again as subpatch */
  t_glist*res=patcherize_makesub(cnv, name, 0,
				 X, Y, xmin, ymin, xmax, ymax, xwin, ywin,
				 connections, maxdollarg);
  if(save2file)*save2file=0;
  s__X.s_thing = boundx;
  s__N.s_thing = boundn;
  binbuf_free(b);
  return res;
}
static t_glist*patcherize_makesub(t_canvas*cnv,
				  const char* name, /* subpatch name of filename */
				  int*save2file_,
				  int X, int Y,
				  int xmin, int ymin, int xmax, int ymax,
				  int xwin, int ywin,
				  t_patcherize_connections*connections,
				  unsigned int maxdollarg) {
  t_binbuf*b=NULL;
  t_gobj*result=NULL;
  t_patcherize_connection*iolets=NULL;
  int x, y;
  int width=xmax-xmin;
  int height=ymax-ymin;
  int save2file=(save2file_)?*save2file_:0;

  /* save and clear bindings to symbols #a, $N, $X; restore when done */
  t_pd *boundx = s__X.s_thing, *boundn = s__N.s_thing;
  s__X.s_thing = &cnv->gl_pd;
  s__N.s_thing = &pd_canvasmaker;

  if(width<200) width=200;
  if(height<100)height=100;

  if (!name || !*name)save2file=0;
  if(save2file && strcmp(name + strlen(name) - 3, ".pd")) {
    /* not a Pd patch */
    save2file=0;
  }

  b=binbuf_new();
  if (save2file) {
    // #N canvas 4 49 450 300 10;
    binbuf_addv(b, "ssiiiii;", gensym("#N"), gensym("canvas"), xwin+xmin, ywin+ymin, width, height, 10);
  } else {
    binbuf_addv(b, "ssiiiisi;", gensym("#N"), gensym("canvas"), xwin+xmin, ywin+ymin, width, height, gensym(name), 0);
  }

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
  if(save2file) {
  } else {
    binbuf_addv(b, "ssiiss;", gensym("#X"), gensym("restore"), X, Y, gensym("pd"), gensym(name));
  }

  if(save2file) {
    /* save the binbuf to file */
    char dirbuf[MAXPDSTRING], *nameptr;
    char objname[MAXPDSTRING];
    int fd;
    int len=strlen(name) -3 ;
    unsigned int i;
    strncpy(objname, name, MAXPDSTRING-1);
    objname[MAXPDSTRING-1]=0;
    if(len>0 && len<MAXPDSTRING)objname[len]=0;
    nameptr=strrchr(name, '/');
    nameptr=nameptr?(nameptr+1):name;

    if(binbuf_write(b, name, "", 0)) {
      /* things went wrong, try again as subpatch */
      return patcherize_makesub_tryagain(cnv, objname, save2file,
					 X, Y, xmin, ymin, xmax, ymax, xwin, ywin,
					 connections, maxdollarg,
					 b, boundx, boundn);
    }
    /* and instantiate the file as an abstraction*/
    if ((fd = canvas_open(cnv, nameptr, "",
			  dirbuf, &nameptr, MAXPDSTRING, 0)) >= 0) {
      sys_close(fd);
      nameptr[-1]='/';
      if(!strcmp(dirbuf, name)) {
	strncpy(objname, nameptr, MAXPDSTRING-2);
	objname[strlen(nameptr)-3]=0; // strip away ".pd".extension
      }
    }
    binbuf_clear(b);
    binbuf_addv(b, "ssiis", gensym("#X"), gensym("obj"), X, Y, gensym(objname));
    for(i=1; i<=maxdollarg; i++) {
      t_atom a;
      char dollstring[MAXPDSTRING];
      snprintf(dollstring, MAXPDSTRING-1, "$%d", i);
      dollstring[MAXPDSTRING-1]=0;
      SETSYMBOL(&a, gensym(dollstring));
      binbuf_add(b, 1, &a);
    }
    binbuf_addsemi(b);
  }
  binbuf_eval(b, 0,0,0);
  binbuf_free(b);

  s__X.s_thing = boundx;
  s__N.s_thing = boundn;

  /* the new object is the last in the parent's glist */
  for(result=cnv->gl_list; result->g_next;) result=result->g_next;

  if(save2file_)*save2file_=save2file;
  return pd_checkglist(&(result->g_pd));
}

static void canvas_patcherize(t_glist*cnv, t_symbol*s) {
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
  const char*name = "/*patcherized*/";
  int save2file=0;
  int maxdollarg=0;

  if(NULL == cnv)return;
  xmin=ymin=INT_MAX;
  xmax=ymax=INT_MIN;

  if (s && s->s_name && *s->s_name) {
    name=s->s_name;
    save2file=1;
  }

  /* store all the selected objects.
   * this needs to be done because the GUI-cleanup in glist_suspend_editor()
   * will undo any selection...
   */
  gobjs=getbytes(0*sizeof(*gobjs));
  for(gobj=cnv->gl_list; gobj; gobj=gobj->g_next) {
    if(glist_isselected(cnv, gobj)) {
      t_object*obj=pd_checkobject(&gobj->g_pd);
      if(!include_in_patcherization(obj)){
	/* deselect excluded objects */
	glist_deselect(cnv, gobj);
	continue;
      }
      if(obj) {
	int dollarg=get_maxdollarg(obj);
	int x=obj->te_xpix;
	int y=obj->te_ypix;
	xpos+=x;
	ypos+=y;
	if(xmin>x)xmin=x;if(xmax<x)xmax=x;
	if(ymin>y)ymin=y;if(ymax<y)ymax=y;
	if(dollarg>maxdollarg)maxdollarg=dollarg;
      }
      gobjs=resizebytes(gobjs, (objcount)*sizeof(*gobjs), (objcount+1)*sizeof(*gobjs));
      gobjs[objcount]=gobj;
      objcount++;
    }
  }

  /* if nothing is selected, we are done... */
  if(!objcount) {
    freebytes(gobjs,objcount * sizeof(*gobjs));
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
  to=patcherize_makesub(cnv, name, &save2file,
			xpos/objcount, ypos/objcount,
			xmin, ymin, xmax+50, ymax+150,
			cnv->gl_screenx1,cnv->gl_screeny1,
			connections, maxdollarg);
  if(!to)
    goto cleanup;

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

  if(save2file) {
    pd_typedmess(to, gensym("menusave"), 0, 0);
  }
  canvas_dirty(cnv, 1);
 cleanup:
  /* cleanup */
  free_connections(connections);
  freebytes(gobjs,objcount * sizeof(*gobjs));

  /* restore state */
  glist_resume_editor(cnv, editFrom);
  canvas_redraw(cnv);
  canvas_resume_dsp(dspstate);
}

void patcherize_setup(void)
{
  if(NULL==canvas_class)return;
  iemguts_boilerplate("patcherize - turn objects into a subpatch", 0);

  if(NULL==zgetfn(&canvas_class, gensym("patcherize")))
    class_addmethod(canvas_class, (t_method)canvas_patcherize, gensym("patcherize"), A_DEFSYM, 0);
}
