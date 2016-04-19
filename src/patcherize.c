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
 *
 * LATER:
 *  - save selection to file
 *
 */

#include "iemguts.h"

#include "g_canvas.h"
#include <string.h>

/* ------------------------- patcherize ---------------------------- */

static void print_glist(t_glist*glist) {
  t_gobj*obj = NULL;
  if(NULL == glist)return;
  post("\t%p", glist);
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

static t_glist*patcherize_makesub(t_canvas*cnv, const char* name, int x, int y) {
  t_binbuf*b=NULL;
  t_gobj*result=NULL;
  char subpatch_text[MAXPDSTRING];

  /* save and clear bindings to symbols #a, $N, $X; restore when done */
  t_pd *boundx = s__X.s_thing, *boundn = s__N.s_thing;

  s__X.s_thing = &cnv->gl_pd;
  s__N.s_thing = &pd_canvasmaker;

  snprintf(subpatch_text, MAXPDSTRING-1,
	   "#N canvas 0 0 450 300 %s 0;#X restore %d %d pd %s;",
	   name, x, y, name);
  subpatch_text[MAXPDSTRING-1]=0;
  b=binbuf_new();
  binbuf_text(b, subpatch_text, strnlen(subpatch_text, MAXPDSTRING-1));
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
  t_gobj*obj = NULL, *last=NULL;
  int objcount=0;
  t_gobj**objs=0;
  t_glist*to;
  int i=0;
  int xpos=0, ypos=0;
  if(NULL == cnv)return;

  /* store all the selected objects.
   * this needs to be done because the GUI-cleanup in glist_suspend_editor()
   * will undo any selection...
   */
  objs=getbytes(0*sizeof(*objs));
  for(obj=cnv->gl_list; obj; obj=obj->g_next) {
    if(glist_isselected(cnv, obj)) {
      t_object*tob=pd_checkobject(obj);
      if(tob) {
	xpos+=tob->te_xpix;
	ypos+=tob->te_ypix;
      }
      objs=resizebytes(objs, (objcount)*sizeof(*objs), (objcount+1)*sizeof(*objs));
      objs[objcount]=obj;
      objcount++;
    }
  }
  /* if nothing is selected, we are done... */
  if(!objcount) {
    freebytes(objs,0*sizeof(*objs));
    return;
  }

  dspstate=canvas_suspend_dsp();

  /* create a new sub-patch to pacherize into */
  to=patcherize_makesub(cnv, "*patcherized*", xpos/objcount, ypos/objcount);

  editFrom=glist_suspend_editor(cnv);

  for(i=0; i<objcount; i++) {
    t_gobj*ob2 = NULL;
    int doit=0;
    obj=objs[i];
    for(ob2=cnv->gl_list; ob2; last=ob2, ob2=ob2->g_next) {
      if (obj == ob2) {
	doit=1;
	break;
      }
    }
    if (!doit)continue;

    /* remove the object from the 'from'-canvas */
    if (last)
      last->g_next = obj->g_next;
    else
      cnv->gl_list = obj->g_next;

    /* append it to the 'to'-canvas */
    if(to->gl_list) {
      for(ob2=to->gl_list; ob2 && ob2->g_next;) ob2=ob2->g_next;
      ob2->g_next = obj;
    } else {
      to->gl_list = obj;
    }
    obj->g_next = 0;

    glist_resume_editor(cnv, editFrom);
  }
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
