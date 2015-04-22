MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = datapath/hal datapath/pipeline

lib_LTLIBRARIES = librofl_datapath.la
librofl_datapath_la_SOURCES = ../rofl_datapath.c
librofl_datapath_la_LIBADD =						\
				datapath/pipeline/librofl_pipeline.la	\
				datapath/hal/librofl_hal.la		\
				-lpthread

ROFL_LIBRARY_VERSION = 1:1:1

librofl_datapath_la_LDFLAGS = -version-info $(ROFL_LIBRARY_VERSION)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rofl_datapath.pc

#AM_LDFLAGS= -static
AM_CPPFLAGS= -fPIC

