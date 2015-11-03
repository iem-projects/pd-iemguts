==============================================================================
		IEMguts
==============================================================================

whatsit::
---------
IEMguts is an extension-library (aka: external, plugin) for 
miller.s.puckette's realtime computer-music environment "Pure Data"; 
it is of no use without Pure Data!

IEMguts is a collection of low level objects that deal with the infrastructure
to build better abstractions.
they might be of no use on their own...


danger::
--------
IEMguts often deal with non-official internals of Pure data.
these internals might change in future versions of Pd, rendering IEMguts
unusable.
always make sure that you are using IEMguts with the same version of Pd as it
was compiled with! else you might experience non-functional, crashing or
exploding intestines.

installation::
--------------
#2> make
#3> make install

note: IEMguts depends on some internal headers of Pd.
therefore you might have to specify the full path to your Pd-sources
using the 'pdincludepath' make-variable should do the trick.
something like:
#2> make pdincludepath=/home/me/src/pd-0.47-0/src
should do the trick.
(if you happen to need to the path to the Pd-binaries as well, you
can additionally use the 'pdbinpath' variable.)

license::
---------
this software is released under the GNU General Public License v2 or later.
you can find the full text of this license in the GnuGPL.txt file that must
be shipped with this software.

authors::
---------
this software is 
copyleft 2007- by 
	IOhannes m zmölnig <zmoelnig [at] iem [dot] at>,
	Institute of Electronic Music and Acoustics,
	University of Music and Dramatic Arts, Graz, Austria

