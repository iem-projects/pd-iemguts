# META helper plugin for patcherize-selection
# META DESCRIPTION adds menu to tell the 'patcherize' library to doit
# META AUTHOR IOhannes m zmölnig <zmoelnig@umlaeute.mur.at>
# META VERSION 0.1

package require pdwindow 0.1
if [catch {
    package require msgcat
    ::msgcat::mcload po
}] { puts "iemguts::patcherize: i18n failed" }

namespace eval ::iemguts::patcherize:: {
    variable label
    proc focus {winid state} {
	set menustate [expr $state?"normal":"disabled"]
	.menubar.edit entryconfigure "$::iemguts::patcherize::label" -state $menustate
    }
    proc register {} {
	# create an entry for our "print2svg" in the "file" menu
	set ::iemguts::patcherize::label [_ "Patcherize Selection"]
	set accelerator $::pd_menus::accelerator
	set mymenu .menubar.edit
	if {$::windowingsystem eq "aqua"} {
	    set inserthere 8
	    #set accelerator "$accelerator+Shift"
	} else {
	    set inserthere 8
	    #set accelerator "Shift+$accelerator"
	}
	$mymenu insert $inserthere command \
	    -label $::iemguts::patcherize::label \
	    -state disabled \
	    -accelerator "$accelerator+P" \
	    -command {	menu_send $::focused_window patcherize }
	bind all <$::modifier-Key-P> {menu_send %W patcherize}
	bind PatchWindow <FocusIn> "+::iemguts::patcherize::focus %W 1"
	bind PdWindow    <FocusIn> "+::iemguts::patcherize::focus %W 0"

	# attempt to load the 'patcherize' library from iemguts
	# (that does all the work)
	pdsend "pd-_float_template declare -lib iemguts/patcherize -stdlib iemguts/patcherize -lib patcherize -stdlib patcherize"

	pdtk_post "loaded iemguts::patcherize-plugin\n"
    }
}


::iemguts::patcherize::register
