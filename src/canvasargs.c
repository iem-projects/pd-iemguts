/******************************************************
 *
 * canvasargs - implementation file
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
 * this object provides a way to manipulate the parent-patches arguments (and name!)
 * usage:
 *   + put this object into an abstraction
 *   + put the abstraction in a patch
 *   + send the object a _list_ of arguments
 *    + the next time the patch (wherein the abstraction that holds this object lives)
 *      is saved, it will be saved with the new arguments instead of the old ones!
 *    - example: "list 2 3 4" will save the object as [<absname> 2 3 4]
 *   + you can also change the abstraction name itself by using a selector other than "list"
 *    - example: "bonkers 8 9" will save the object as [bonkers 8 9] regardless of it's original name
 *    - use with care!
 *
 * nice, eh?
 */

#include "iemguts.h"
#include "g_canvas.h"

EXTERN int canvas_getdollarzero(void);


/* ------------------------- canvasargs ---------------------------- */

static t_class *canvasargs_class;

typedef struct _canvasargs
{
  t_object  x_obj;

  t_canvas  *x_canvas;
} t_canvasargs;

static void canvasargs_set(t_canvasargs *x, int argc, t_atom*argv, int dollsyms)
{
  t_binbuf*b=0, *unescapebuf=0;
  t_atom name[1];

  if(!x || !x->x_canvas) return;
  b=x->x_canvas->gl_obj.te_binbuf;
  if(!b)return;

  /* get the name from the canvas */
  SETSYMBOL(name, atom_getsymbol(binbuf_getvec(b)));
  binbuf_clear(b);
  binbuf_add(b, 1, name);

  if(dollsyms) {
    /* try to add symbols with dollars as A_DOLLAR resp. A_DOLLSYM */
    unescapebuf = binbuf_new();
    binbuf_restore(unescapebuf, argc, argv);
    argc = binbuf_getnatom(unescapebuf);
    argv = binbuf_getvec(unescapebuf);
  }

  binbuf_add(b, argc, argv);

  if(unescapebuf)
    binbuf_free(unescapebuf);
}

static void canvasargs_list(t_canvasargs *x, t_symbol*s, int argc, t_atom*argv)
{
  (void)s;
  canvasargs_set(x, argc, argv, 1);
}
static void canvasargs_setraw(t_canvasargs *x, t_symbol*s, int argc, t_atom*argv) {
  (void)s;
  return canvasargs_set(x, argc, argv, 0);
}

static void canvasargs_doit(t_canvasargs *x, int expanddollargs)
{
  int argc=0;
  t_atom*argv=0;
  t_binbuf*b=0,*b1=0;

  if(!x->x_canvas) return;
  b=x->x_canvas->gl_obj.te_binbuf;
  if (!b) {
    canvas_setcurrent(x->x_canvas);
    canvas_getargs(&argc, &argv);
    canvas_unsetcurrent(x->x_canvas);
  } else {
    /*
     * expanding dollargs:
     * if there's no parent canvas, A_DOLLAR is expanded to 0, A_DOLLSYM to the literal
     * unexpanded dollargs:
     *  with A_DOLLSYM, take the literal, with A_DOLLAR construct the '$%d' string
     */
    int i;
    t_canvas*parent = x->x_canvas->gl_owner;
    int pargc=0;
    t_atom*pargv=0;

    if(parent) {
      canvas_setcurrent(parent);
      canvas_getargs(&pargc, &pargv);
    }

    b1 = binbuf_duplicate(b);
    argc=binbuf_getnatom(b1)-1;
    argv=binbuf_getvec(b1)+1;
    for(i=0; i<argc; i++) {
      t_symbol*s = 0;
      switch(argv[i].a_type) {
      default: break;
      case A_DOLLAR:
	if(expanddollargs) {
	  /* 0 if out-of-range, otherwise copy parent atom */
	  int idx = argv[i].a_w.w_index-1;
	  if (idx == -1) {
	    SETFLOAT(argv+i, canvas_getdollarzero());
	  } else if((idx < 0) || (idx >= pargc)) {
	    SETFLOAT(argv+i, 0);
	  } else {
	    argv[i] = pargv[idx];
	  }
	} else {
	  /* create the dollarg-string */
	  char buf[MAXPDSTRING];
	  atom_string(argv+i, buf, MAXPDSTRING-2);
	  buf[MAXPDSTRING-1]=0;
	  SETSYMBOL(argv+i, gensym(buf));
	}
	break;
      case A_DOLLSYM:
	if(expanddollargs)
	  s = binbuf_realizedollsym(argv[i].a_w.w_symbol, pargc, pargv, 0);
	if(!s)
	  s = argv[i].a_w.w_symbol;
	SETSYMBOL(argv+i, s);
        break;
      }
    }
    if(parent) {
      canvas_unsetcurrent(parent);
    }
  }

  if(argv)
    outlet_list(x->x_obj.ob_outlet, &s_list, argc, argv);
  if(b1)
    binbuf_free(b1);
}

static void canvasargs_bang(t_canvasargs *x) {
  return canvasargs_doit(x, 1);
}
static void canvasargs_raw(t_canvasargs *x) {
  return canvasargs_doit(x, 0);
}

static void canvasargs_free(t_canvasargs *x)
{
  x->x_canvas = 0;
}

static void *canvasargs_new(t_floatarg f)
{
  t_canvasargs *x = (t_canvasargs *)pd_new(canvasargs_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);

  int depth=(int)f;
  if(depth<0)depth=0;

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }

  x->x_canvas = canvas;

  outlet_new(&x->x_obj, 0);
  return (x);
}

void canvasargs_setup(void)
{
  iemguts_boilerplate("[canvasargs]", 0);
  canvasargs_class = class_new(gensym("canvasargs"), (t_newmethod)canvasargs_new,
                               (t_method)canvasargs_free, sizeof(t_canvasargs), 0, A_DEFFLOAT, 0);
  class_addlist(canvasargs_class, (t_method)canvasargs_list);
  class_addbang(canvasargs_class, (t_method)canvasargs_bang);
  class_addmethod(canvasargs_class, (t_method)canvasargs_raw, gensym("raw"), 0);
  class_addmethod(canvasargs_class, (t_method)canvasargs_setraw, gensym("setraw"), A_GIMME, 0);
}
