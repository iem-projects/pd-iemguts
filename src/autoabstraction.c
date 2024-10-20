/* Copyright (c) 2008 IOhannes m zmölnig @ IEM
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," that comes with Pd.
 ******************************************************
 *
 * license: GNU General Public License v.2 (or later)
 *
 ******************************************************/

/*
 * this code adds an external "loader" to Miller S. Puckette's "pure data",
 * which allows the on-the-fly creation of abstractions by just creating an
 * object which is of a yet unknown file and for which no abstraction-file exists
 *
 * the new abstraction should be minimal and eventually be loaded from a template
 *
 * possible default abstraction: '#N canvas 0 0 450 300 10; #X vis 1;'
 *
 * TODO: currently the template-abstraction is only initialized once.
 * this means that we cannot change the template after loading of "autoabstraction"
 * - FIX(LATER): add a "reload"-message to the [autoabstraction] object
 * - FIX(ALT): check whether the template-file has changed and reload it
 */

#include "iemguts.h"

typedef struct _autoabstraction
{
  t_object x_obj;
} t_autoabstraction;
static t_class *autoabstraction_class;

static int s_state=0;

/* this is the name of the filename that get's loaded as a default template for new abstractions */
static const char*s_templatefilename="autoabstraction.template";
/* if the loading above fails, we resort to a simple default abstraction that automatically opens up */
/* LATER: make the font-size the same as the one used by Pd */
static char*s_templatestring="#N canvas 0 0 450 300 10; #X vis 1;";

# include "s_stuff.h"
# include "g_canvas.h"

# include <stdio.h>
# include <string.h>
# include <unistd.h>

# ifdef UNISTD
#  include <stdlib.h>
#  include <unistd.h>
# endif
# ifdef _WIN32
#  include <io.h>
#  include <windows.h>
# endif

#if (PD_MINOR_VERSION < 42)
 /* definitions taken from s_loader.c, since they weren't in header orignally */
 typedef int (*loader_t)(t_canvas *canvas, char *classname);
 void sys_register_loader(loader_t loader);
 void class_set_extern_dir(t_symbol *s);
#endif

static t_binbuf*s_bb=0;

static void autoabstraction_createpatch(t_canvas*canvas, char*classname) {
  (void)canvas;
  if(!s_state) {
    /* autoabstraction turned off... */
    return;
  }
  if(s_bb) {
    char name[MAXPDSTRING];
    snprintf(name, MAXPDSTRING, "%s.pd", classname);

    binbuf_write(s_bb, name, "", 0);
  } else {
    iemguts_verbose(1, "[autoabstraction]: no template");
  }
}

/**
 * the loader
 *
 * @param canvas the context of the object to be created
 * @param classname the name of the object (external, library) to be created
 * @return 1 on success, 0 on failure
 */
static int autoabstraction_loader_legacy(t_canvas *canvas, char *classname)
{
  /* check whether there is an abstraction with the given <classname> within the scope of <canvas> */
  int fd=0;
  char dirbuf[MAXPDSTRING], *nameptr;

  if((fd=canvas_open(canvas, classname, ".pd", dirbuf, &nameptr, MAXPDSTRING, 0)) >= 0)
    {
      /* oops, there's already an abstraction of the given <classname> */
      close(fd);
      return(0);
    }

  /* none! we create one, so Pd can later find it... */
  autoabstraction_createpatch(canvas, classname);

  /* we always fail, because we want Pd to do the real opening of abstractions */
  return 0;
}

static t_pd *do_create_autoabstraction(t_symbol*s, int argc, t_atom *argv) {
  t_pd*obj = 0;
  t_symbol*x_s=gensym("#X");
  t_pd *boundx = x_s->s_thing;
  const char*dirname=".";
  char name[MAXPDSTRING+1];
#if 0
  if (pd_setloadingabstraction(s)) {
    pd_error(obj, "%s: can't load autoabstraction within itself\n", s->s_name);
    return obj;
  }
#endif

  snprintf(name, MAXPDSTRING, "%s.pd", s->s_name);
  name[MAXPDSTRING] = 0;

  canvas_setargs(argc, argv);
  binbuf_evalfile(gensym(name), gensym(dirname));
  if (x_s->s_thing && boundx != x_s->s_thing) {
    //canvas_popabstraction((t_canvas *)(x_s->s_thing));
    t_canvas*cnv = (t_canvas *)(s__X.s_thing);
    obj = &cnv->gl_pd;
    gensym("#A")->s_thing = 0;
    pd_bind(obj, gensym("#A"));
    pd_popsym(obj);
    cnv->gl_loading = 0;
  } else
    obj = pd_newest();

  canvas_setargs(0, 0);
  return (obj);
}

static int autoabstraction_loader(t_canvas *canvas, char *classname, const char*path) {
  if(path)
    return 0;

  autoabstraction_createpatch(canvas, classname);
  class_set_extern_dir(gensym("."));
  class_new(gensym(classname),
      (t_newmethod)do_create_autoabstraction, 0,
      0, 0, A_GIMME, 0);
  class_set_extern_dir(&s_);
  return 1;
}

static void autoabstraction_initialize(void)
{
  if(s_bb)
    binbuf_free(s_bb);

  s_bb=binbuf_new();

  /* try to read a template file */
  if(binbuf_read(s_bb, s_templatefilename, "", 0)) {
    /* if this fails, use the default template */
    size_t length=strlen(s_templatestring);

    binbuf_text(s_bb, s_templatestring, length);
  }
  s_state=1;
}

static void autoabstraction_state(t_autoabstraction*x, t_floatarg f)
{
  int state=(int)(f>=1.0);
  (void)x;
  s_state=state;
}

static void*autoabstraction_new(t_symbol *s, int argc, t_atom *argv)
{
  t_autoabstraction*x = (t_autoabstraction*)pd_new(autoabstraction_class);
  (void)s;

  if(argc&&A_SYMBOL==argv->a_type) {
    s_templatefilename=atom_getsymbol(argv)->s_name;
  }

  autoabstraction_initialize();
  return (x);
}

void autoabstraction_setup(void)
{
  int major, minor, bugfix;
  sys_getversion(&major, &minor, &bugfix);

  /* relies on t.grill's loader functionality, fully added in 0.40 */
  iemguts_boilerplate("automatic abstraction creator", 0);
  autoabstraction_initialize();
  if (major>0 || minor >=47) {
    sys_register_loader((loader_t)autoabstraction_loader);
  } else {
    sys_register_loader((loader_t)autoabstraction_loader_legacy);
  }

  autoabstraction_class = class_new(gensym("autoabstraction"), (t_newmethod)autoabstraction_new, 0, sizeof(t_autoabstraction), 0, A_GIMME, 0);
  class_addfloat(autoabstraction_class, (t_method)autoabstraction_state);
}
