#!/usr/bin/make -f
# Makefile to the 'iemguts' library for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules (https://github.com/pure-data/pd-lib-builder).

lib.name = iemguts

# special file that does not provide a class
lib.setup.sources = 

# all other C and C++ files in subdirs are source files per class
# (alternatively, enumerate them by hand)
class.sources = $(filter-out $(lib.setup.sources),$(wildcard src/*.c))

datafiles = \
$(wildcard help/*.pd) \
LICENSE.txt \
README.txt \
iemguts-meta.pd

datadirs =  examples

cflags = -DVERSION=$(lib.version)

DATE_FMT = %Y/%m/%d at %H:%M:%S UTC
ifdef SOURCE_DATE_EPOCH
    BUILD_DATE ?= $(shell date -u -d "@$(SOURCE_DATE_EPOCH)" "+$(DATE_FMT)" 2>/dev/null || date -u -r "$(SOURCE_DATE_EPOCH)" "+$(DATE_FMT)" 2>/dev/null || date -u "+$(DATE_FMT)")
endif
ifdef BUILD_DATE
cflags += -DBUILD_DATE='"$(BUILD_DATE)"'
endif

################################################################################
### pdlibbuilder ###############################################################
################################################################################

# This Makefile is based on the Makefile from pd-lib-builder written by
# Katja Vetter. You can get it from:
# https://github.com/pure-data/pd-lib-builder
PDLIBBUILDER_DIR=pd-lib-builder/
include $(firstword $(wildcard $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder Makefile.pdlibbuilder))
