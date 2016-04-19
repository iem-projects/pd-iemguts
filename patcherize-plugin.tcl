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
    pdsend "$mytoplevel patcherize"
}

proc focus {winid state} {
    set menustate [expr $state?"normal":"disabled"]
    .menubar.edit entryconfigure "$::iemguts::patcherize::label" -state $menustate
}

proc register {} {
    # create an entry for our "print2svg" in the "file" menu
    set ::iemguts::patcherize::label [_ "Patcherize Selection"]
    set mymenu .menubar.edit
    if {$::windowingsystem eq "aqua"} {
        set inserthere 8
    } else {
        set inserthere 8
    }
    #$mymenu insert $inserthere separator
    $mymenu insert $inserthere command \
        -label $::iemguts::patcherize::label \
        -state disabled \
        -command {::iemguts::patcherize::selection2patch $::focused_window}
    # bind all <$::modifier-Key-s> {::deken::open_helpbrowser .helpbrowser2}
    bind PatchWindow <FocusIn> "+::iemguts::patcherize::focus %W 1"
    bind PdWindow    <FocusIn> "+::iemguts::patcherize::focus %W 0"

    pdtk_post "loaded iemguts::patcherize-plugin\n"
}

}


::iemguts::patcherize::register
