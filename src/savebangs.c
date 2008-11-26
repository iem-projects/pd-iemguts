
/******************************************************
 *
 * propertybang - implementation file
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
 * this object outputs a bang when the savfn of the parent abstraction is called
 */

/*
 * LATER sketch: 
 *     monkey-patching the parent canvas so that stores a local function table
 *     then modify this function-table so that the savefn points to us
 *   on call: we call the parents savefunction and output bangs
 */

/* 
 * TODO: how does this behave in sub-patches?
 *      -> BUG: the depth should _really_ refer to the abstraction-depth 
 *              else we get weird duplicates (most likely due to the "$0" trick
 *
 * TODO: make [savebangs] do something on top-level
 *    that is: if the patch the [savebangs] is in gets saved, [savebangs] will fire
 *    think (a little) about how the args to savebang have to look like to make it compatible with the [canvas*] stiff
 *
 * TODO: maintain our own list of [savebangs] to be called per abstraction rather than using the $0-trick
 */

#include "m_pd.h"
#include "g_canvas.h"

/* ------------------------- help methods ---------------------------- */


typedef struct _savefuns {
  t_class*class;
  t_savefn savefn;

  struct _savefuns *next;
} t_savefuns;

static t_savefuns*s_savefuns=0;


static t_savefn find_savefn(const t_class*class) 
{
  t_savefuns*fun=s_savefuns;
  if(0==s_savefuns || 0==class)
    return 0;
  for(fun=s_savefuns; fun; fun=fun->next) {
    if(class == fun->class) {
      return fun->savefn;
    }
  }

  return 0;
}
static void add_savefn(t_class*class)
{
  if(0!=find_savefn(class)) {
    return;
  } else {
    t_savefuns*sfun=(t_savefuns*)getbytes(sizeof(t_savefuns));
    sfun->class=class;
    sfun->savefn=class_getsavefn(class);
    sfun->next=0;

    if(0==s_savefuns) {
      s_savefuns=sfun;
    } else {
      t_savefuns*sfp=s_savefuns;
      while(sfp->next)
        sfp=sfp->next;
      sfp->next = sfun;      
    }
  }
}



/* ------------------------- savebangs ---------------------------- */

static t_class *savebangs_class;

typedef struct _savebangs
{
  t_object  x_obj;

  t_symbol *x_d0;
  t_outlet *x_pre, *x_post;

  t_savefn x_parentsavefn;
} t_savebangs;


static void savebangs_free(t_savebangs *x)
{
  /* unpatch the parent canvas */
  if(x->x_d0) {
    pd_unbind(&x->x_obj.ob_pd, x->x_d0);
  }
}

static void orig_savefn(t_gobj*z, t_binbuf*b)
{
   t_class*class=z->g_pd;
    t_savefn savefn=find_savefn(class);
    if(savefn) {
      savefn(z, b);
    }
}

static void savebangs_savefn(t_gobj*z, t_binbuf*b) {
  /* argh: z is the abstraction! but we need to access ourselfs!
   * we handle this by binding to a special symbol. e.g. "$0 savebangs"
   * (we use the space between in order to make it hard for the ordinary user 
   * to use this symbol for other things...
   */

  /* alternatively we could just search the abstraction for all instances of savebangs_class
   * and bang these;
   * but using the pd_bind-trick is simpler for now
   * though not as sweet, as somebody could use our bind-symbol for other things...
   */

  t_symbol*s_d0=canvas_realizedollar((t_canvas*)z, gensym("$0 savebangs"));
  t_atom ap[2];
  SETPOINTER(ap+0, (t_gpointer*)z);
  SETPOINTER(ap+1, (t_gpointer*)b);

  if(s_d0->s_thing) {
    pd_list(s_d0->s_thing, &s_list, 2, ap);
  } else {
    orig_savefn(z, b);
  }
}

static void savebangs_list(t_savebangs *x, t_symbol*s, int argc, t_atom*argv)
{
  if(argv[0].a_type == A_POINTER && argv[1].a_type == A_POINTER) {    
    t_gobj *z    =(t_gobj*)  argv[0].a_w.w_gpointer;
    t_binbuf*b   =(t_binbuf*)argv[1].a_w.w_gpointer;

    outlet_bang(x->x_pre);
    orig_savefn(z, b);
    outlet_bang(x->x_post);
  }
}

static void *savebangs_new(t_floatarg f)
{
  t_savebangs *x = (t_savebangs *)pd_new(savebangs_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
  t_class *class = 0;

  
  int depth=(int)f;
  if(depth<0)depth=0;

  while(depth && canvas) {
    canvas=canvas->gl_owner;
    depth--;
  }
  
  if(canvas) {
    class=((t_gobj*)canvas)->g_pd;
    x->x_d0=canvas_realizedollar(canvas, gensym("$0 savebangs"));
    pd_bind(&x->x_obj.ob_pd, x->x_d0);
    
    add_savefn(class);
    class_setsavefn(class, savebangs_savefn);
  } else {
    x->x_d0=0;
  }

  x->x_pre=outlet_new(&x->x_obj, &s_bang);
  x->x_post=outlet_new(&x->x_obj, &s_bang);

  return (x);
}

void savebangs_setup(void)
{
  savebangs_class = class_new(gensym("savebangs"), (t_newmethod)savebangs_new,
                              (t_method)savebangs_free, sizeof(t_savebangs), CLASS_NOINLET, A_DEFFLOAT, 0);
  class_addlist(savebangs_class, savebangs_list);
}
