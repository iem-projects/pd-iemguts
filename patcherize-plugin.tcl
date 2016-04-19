# META convert a patch to a picture (SVG)
# META DESCRIPTION exports your patch as an SVG-image
# META AUTHOR IOhannes m zmölnig <zmoelnig@umlaeute.mur.at>
# META VERSION 0.1

package require pdwindow 0.1
if [catch {
    package require msgcat
    ::msgcat::mcload po
}] { puts "iemguts::patcherize: i18n failed" }

namespace eval ::iemguts::patcherize:: {
    variable label
    proc export {mytoplevel filename} {
        can2svg::canvas2file [tkcanvas_name $mytoplevel] $filename
    }
    proc is_patchwindow {w} {
        #expr {[winfo toplevel $w] eq $w && ![catch {$w cget -menu}]}
        expr {[winfo class $w] eq "PatchWindow"}
    }

    proc selection2patch {mytoplevel} {
	puts "patcherize $mytoplevel"
	pdsend "$mytoplevel patcherize"
    }

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
	    -command {::iemguts::patcherize::selection2patch $::focused_window}
	bind all <$::modifier-Key-P> {menu_send %W patcherize}
	bind PatchWindow <FocusIn> "+::iemguts::patcherize::focus %W 1"
	bind PdWindow    <FocusIn> "+::iemguts::patcherize::focus %W 0"

	pdtk_post "loaded iemguts::patcherize-plugin\n"
    }
}


::iemguts::patcherize::register
