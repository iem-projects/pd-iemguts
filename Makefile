#!/usr/bin/make -f
# Makefile for pure data externals in lib creb.
# Needs Makefile.pdlibbuilder to work (https://github.com/pure-data/pd-lib-builder)

lib.name = iemguts

# special file that does not provide a class
lib.setup.sources = 

# all other C and C++ files in subdirs are source files per class
# (alternatively, enumerate them by hand)
class.sources = $(filter-out $(lib.setup.sources),$(wildcard src/*.c))

datafiles = \
$(wildcard help/*-help.pd) \
LICENSE.txt \
README.txt \
iemguts-meta.pd

datadirs =  examples

################################################################################
### pdlibbuilder ###############################################################
################################################################################


# Include Makefile.pdlibbuilder from this directory, or else from externals
# root directory in pd-extended configuration.

include $(firstword $(wildcard Makefile.pdlibbuilder ../Makefile.pdlibbuilder))

