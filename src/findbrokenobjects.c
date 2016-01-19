
/******************************************************
 *
 * findbrokenobjects - implementation file
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
 * find broken objects (objects that could not be created)
 * these objects are of class 'text_class'

 objsrc = pd_checkobject(&src->g_pd);
 if (objsrc && pd_class(&src->g_pd) == text_class && objsrc->te_type == T_OBJECT) {
   // 'src' is a broken object
 }
 */

#include "iemguts.h"
#include "g_canvas.h"
#include "m_imp.h"

#include <string.h>

int glist_getindex(t_glist *x, t_gobj *y);

/* ------------------------- findbrokenobjects ---------------------------- */

static t_class *findbrokenobjects_class;

typedef struct _findbrokenobjects
{
  t_object  x_obj;
  t_outlet *x_out;
  t_canvas *x_parent;   // the canvas we are acting on
} t_findbrokenobjects;

extern t_class *text_class;
extern t_class *canvas_class;

static void print_obj(const char*prefix, t_object *obj) {
  int ntxt;
  char *txt;
  t_binbuf*bb=obj->te_binbuf;
  binbuf_gettext(bb, &txt, &ntxt);
  pd_error(obj, "%s%p\t%s", prefix, obj, txt);
}

static void findbrokenobjects_doit(t_canvas*cnv) {
  t_gobj*src;
  for (src = cnv->gl_list; src; src = src->g_next) { /* traverse all objects in canvas */
    t_object*obj=pd_checkobject(&src->g_pd);
    if (obj && (pd_class(&src->g_pd) == text_class && obj->te_type == T_OBJECT)) {
      // found broken object
      print_obj("broken:\t", obj);
    }
  }
}

static void fbo_iterate(t_canvas*x) {
  // iterate over all top-level canvases
  if(!(x && x->gl_name && x->gl_name->s_name))
    return;
  t_gobj*g=0;

  for(g=x->gl_list;g;g=g->g_next) {
    // iterate over all objects on the canvas
    t_object*ob=pd_checkobject(&g->g_pd);
    t_class*cls=0;

    if(!(ob && ob->te_type == T_OBJECT))
      continue;

    cls=pd_class(&g->g_pd);
    //const char*classname=cls->c_name->s_name;
    if (cls == canvas_class) {
      // this is just another abstraction, recurse into it
      fbo_iterate(ob);
    } else if (cls == text_class) {
      /* broken object */
      int ntxt;
      char *txt;
      t_binbuf*bb=ob->te_binbuf;
      binbuf_gettext(bb, &txt, &ntxt);
      pd_error(ob, "[%s] broken object!", txt);
      freebytes(txt, ntxt);
    }
  }
}

static void findbrokenobjects_bang(t_findbrokenobjects *x) {
  // find all broken objects in the current patch
  if(x->x_parent) {
    fbo_iterate(x->x_parent);
  } else {
    t_canvas *c;
    for (c = pd_getcanvaslist(); c; c = c->gl_next) {
      const char*name=c->gl_name->s_name;
      /* only allow names ending with '.pd'
       * (reject template canvases)
       */
      const int len=strlen(name);
      if(len>3 && name[len-3]=='.' && name[len-2]=='p' && name[len-1]=='d') {
	fbo_iterate(c);
      } //else post("canvas: %s", name);
    }
  }
}

static void findbrokenobjects_free(t_findbrokenobjects *x)
{
  outlet_free(x->x_out);
}

static void *findbrokenobjects_new(t_symbol*s, int argc, t_atom*argv)
{
  t_findbrokenobjects *x = (t_findbrokenobjects *)pd_new(findbrokenobjects_class);
  x->x_parent=0;
  if(argc==1 && argv->a_type == A_FLOAT) {
    int depth=atom_getint(argv);
    t_glist *glist=(t_glist *)canvas_getcurrent();
    if(depth>=0) {
      t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
      while(depth && canvas) {
	canvas=canvas->gl_owner;
	depth--;
      }
      if(canvas)
	x->x_parent = canvas;
    }
  }
  post("findbrokenobj @ %p", x->x_parent);

  x->x_out = outlet_new(&x->x_obj, &s_float);
  return (x);
}

static char fbo_file[];
static void fbo_persist(void) {
  static t_pd*fbo_canvas=0;
  if(fbo_canvas)
    return;


  t_binbuf *b = binbuf_new();
  glob_setfilename(0, gensym("_deken_workspace"), gensym("."));
  binbuf_text(b, fbo_file, strlen(fbo_file));
  binbuf_eval(b, &pd_canvasmaker, 0, 0);
  fbo_canvas = s__X.s_thing;
  vmess(s__X.s_thing, gensym("pop"), "i", 0);
  glob_setfilename(0, &s_, &s_);
  binbuf_free(b);
}

void findbrokenobjects_setup(void)
{
  iemguts_boilerplate("[findbrokenobjects]", 0);
  findbrokenobjects_class = class_new(gensym("findbrokenobjects"), (t_newmethod)findbrokenobjects_new,
				     (t_method)findbrokenobjects_free, sizeof(t_findbrokenobjects), 0,
				     A_GIMME, 0);
  class_addbang  (findbrokenobjects_class, (t_method)findbrokenobjects_bang);
  fbo_persist();
}
static char fbo_file[] = "\
canvas 0 0 300 200;\n\
#X obj 20 20 receive __deken_findbroken_objects;\n\
#X obj 20 60 findbrokenobjects;\n\
#X obj 20 80 list prepend plugin-dispatch deken;\n\
#X msg 20 40 unique;\n\
#X obj 20 100 list trim;\n\
#X obj 20 120 s pd;\n\
#X connect 0 0 3 0;\n\
#X connect 1 0 2 0;\n\
#X connect 2 0 4 0;\n\
#X connect 3 0 1 0;\n\
#X connect 4 0 5 0;\n\
";
