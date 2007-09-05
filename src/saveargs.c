
/******************************************************
 *
 * saveargs - implementation file
 *
 * copyleft (c) IOhannes m zm-bölnig-A
 *
 *   2007:forum::f-bür::umläute:2007-A
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
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

#include "m_pd.h"
#include "g_canvas.h"


/* ------------------------- saveargs ---------------------------- */

static t_class *saveargs_class;

typedef struct _saveargs
{
  t_object  x_obj;

  t_canvas  *x_canvas;
} t_saveargs;


static void saveargs_list(t_saveargs *x, t_symbol*s, int argc, t_atom*argv)
{
  t_canvas*c=x->x_canvas;
  t_binbuf*b=0;
  t_atom name[1];

  if(!x->x_canvas) return;
  b=x->x_canvas->gl_obj.te_binbuf;

  if(!b)return;

  if(s==0 || s==gensym("") || s==&s_list || s==&s_bang || s==&s_float || s==&s_symbol || s==&s_) {
    t_atom*ap=binbuf_getvec(b);
    s=atom_getsymbol(ap);
  }
  SETSYMBOL(name, s);
  
  binbuf_clear(b);
  binbuf_add(b, 1, name);
  binbuf_add(b, argc, argv);
}

static void saveargs_free(t_saveargs *x)
{
}

static void *saveargs_new(void)
{
  t_saveargs *x = (t_saveargs *)pd_new(saveargs_class);
  t_glist *glist=(t_glist *)canvas_getcurrent();
  t_canvas *canvas=(t_canvas*)glist_getcanvas(glist);
    
  x->x_canvas = canvas;
  return (x);
}

void saveargs_setup(void)
{
  saveargs_class = class_new(gensym("saveargs"), (t_newmethod)saveargs_new,
    (t_method)saveargs_free, sizeof(t_saveargs), 0, 0);
  class_addanything(saveargs_class, (t_method)saveargs_list);
}
