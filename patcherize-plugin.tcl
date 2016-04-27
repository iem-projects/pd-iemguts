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
    variable label_sub
    variable label_abs
    proc focus {winid state} {
        set menustate [expr $state?"normal":"disabled"]
        .menubar.edit entryconfigure "$::iemguts::patcherize::label_sub" -state $menustate
        .menubar.edit entryconfigure "$::iemguts::patcherize::label_abs" -state $menustate
    }
    ## helper function to pick a filename for saving
    # (why isn't there something like this in Pd-GUI?)
    proc savepanel {initialdir {initialfile ""} {defaultextension ".pd"} {filetypes $::filetypes} } {
	if { "$filetypes" == {$::filetypes} } {
	    set filetypes $::filetypes
	}
        if { ! [file isdirectory $initialdir]} {set initialdir $::env(HOME)}
        set filename [tk_getSaveFile \
			  -defaultextension $defaultextension \
                          -filetypes $filetypes \
                          -initialfile $initialfile \
			  -initialdir $initialdir]
	return $filename
    }

    proc patcherize2file {winid} {
	set filename [::iemguts::patcherize::savepanel "" "" ".pd" [list [list [_ "Pd Files"]          {.pd}  ]]]
	if { $filename != "" } {
	    menu_send $::focused_window "patcherize [enquote_path $filename]"
	}
    }
    proc register {} {
        # create an entry for our "print2svg" in the "file" menu
        set ::iemguts::patcherize::label_sub [_ "SubPatcherize Selection"]
        set ::iemguts::patcherize::label_abs [_ "Patcherize Selection..."]
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
            -label $::iemguts::patcherize::label_sub \
            -state disabled \
            -accelerator "$accelerator+P" \
            -command { menu_send $::focused_window patcherize }
        $mymenu insert [incr inserthere] command \
            -label $::iemguts::patcherize::label_abs \
            -state disabled \
            -command { ::iemguts::patcherize::patcherize2file $::focused_window }

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
