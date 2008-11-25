/* Copyright (c) 2008 IOhannes m zmölnig @ IEM
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," that comes with Pd.  
 */

/*
 * this code adds an external "loader" to Miller S. Puckette's "pure data",
 * which allows the on-the-fly creation of abstractions by just creating an
 * object which is of a yet unknown file and for which no abstraction-file exists
 *
 * the new abstraction should be minimal and eventually be loaded from a template
 *
 * possible default abstraction: '#N canvas 0 0 450 300 10; #X vis 1;'
 *
 */


#ifdef __WIN32__
# define MSW
#endif

#include "m_pd.h"


typedef struct _autoabstraction
{
  t_object x_obj;
} t_autoabstraction;
static t_class *autoabstraction_class;

static char *version = "$Revision: 0.1 $";

/* this is the name of the filename that get's loaded as a default template for new abstractions */
static char*templatefilename="autoabstraction.template";
/* if the loading above fails, we resort to a simple default abstraction that automatically opens up */
static char*templatestring="#N canvas 0 0 450 300 10; #X vis 1;";


#if (PD_MINOR_VERSION >= 40)

#include "s_stuff.h"
#include "g_canvas.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef UNISTD
# include <stdlib.h>
# include <unistd.h>
#endif
#ifdef _WIN32
# include <io.h>
# include <windows.h>
#endif

#ifdef MISSING_LOADER_T
/* definitions taken from s_loader.c, since they weren't in header orignally */
typedef int (*loader_t)(t_canvas *canvas, char *classname);
void sys_register_loader(loader_t loader);
void class_set_extern_dir(t_symbol *s);
#endif


static t_binbuf*aa_bb=0;


static void autoabstraction_save(t_canvas*canvas, char*classname) {
  t_binbuf*bb=0;

  if(!bb)
    bb=aa_bb;

  if(bb) {
    char name[MAXPDSTRING];
    snprintf(name, MAXPDSTRING, "%s.pd", classname);

    binbuf_write(bb, name, "", 0);
  } else {
    verbose(1, "[autoabstraction]: no template");
  }
}


/**
 * the loader
 *
 * @param canvas the context of the object to be created
 * @param classname the name of the object (external, library) to be created
 * @return 1 on success, 0 on failure
 */
static int autoabstraction_loader(t_canvas *canvas, char *classname)
{
  /* check whether there is an abstraction with the given <classname> within the scope of <canvas> */
  int fd=0;
  char dirbuf[MAXPDSTRING], *nameptr;

  if((fd=canvas_open(canvas, classname, ".pd", dirbuf, &nameptr, MAXPDSTRING, 0)) >= 0 ||
     (fd=canvas_open(canvas, classname, ".pat", dirbuf, &nameptr, MAXPDSTRING, 0)) >= 0)
    {
      /* oops, there's already an abstraction of the given <classname> */
      close(fd);
      return(0);
    }


  autoabstraction_save(canvas, classname);


  /* we always fail, because we want Pd to do the real opening of abstractions */
  return 0;
}

static void autoabstraction_initialize(void)
{
  aa_bb=binbuf_new();

  /* try to read a template file */
  if(binbuf_read(aa_bb, templatefilename, "", 0)) {
    /* if this fails, use the default template */
    size_t length=strlen(templatestring);

    binbuf_text(aa_bb, templatestring, length);
  }
}



#endif /* PD_MINOR_VERSION >= 40 */



static void*autoabstraction_new(t_symbol *s, int argc, t_atom *argv)
{
  t_autoabstraction*x = (t_autoabstraction*)pd_new(autoabstraction_class);
  return (x);
}

void autoabstraction_setup(void)
{
  /* relies on t.grill's loader functionality, fully added in 0.40 */
  post("automatic abstraction creator %s",version);  
  post("\twritten by IOhannes m zmoelnig, IEM <zmoelnig@iem.at>");
  post("\tcompiled on "__DATE__" at "__TIME__ " ");
  post("\tcompiled against Pd version %d.%d.%d.%s", PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION, PD_TEST_VERSION);

#if (PD_MINOR_VERSION >= 40)
  autoabstraction_initialize();
  sys_register_loader(autoabstraction_loader);
#else
  error("to function, this needs to be compiled against Pd 0.40 or higher,\n");
  error("\tor a version that has sys_register_loader()");
#endif

  autoabstraction_class = class_new(gensym("autoabstraction"), (t_newmethod)autoabstraction_new, 0, sizeof(t_autoabstraction), CLASS_NOINLET, A_GIMME, 0);
}
