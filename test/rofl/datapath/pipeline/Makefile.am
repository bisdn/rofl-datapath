MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = . monitoring openflow/openflow1x/pipeline

#Copy pipeline files required by pipeline tests 
BUILT_SOURCES = pipe_sources
CLEANFILES = pipe_sources
pipe_sources:
	cp -rf $(top_srcdir)/src/rofl/datapath/pipeline pipeline_

AUTOMAKE_OPTIONS = no-dependencies

export AM_CPPFLAGS= -DROFL_TEST=1

tern_alike_unit_test_SOURCES= tern_alike_test.c \
	openflow/openflow1x/pipeline/pthread_lock.c\
	openflow/openflow1x/pipeline/timing.c\
	openflow/openflow1x/pipeline/pthread_atomic_operations.c\
	openflow/openflow1x/pipeline/memory.c\
	pipeline_/util/logging.c\
	pipeline_/common/ternary_fields.c\
	pipeline_/common/alike_masks.c

tern_alike_unit_test_LDADD= -lcunit -lpthread

#-lrofl_pipeline -lrofl
# we need to link to the local libraries, not the installed ones
# and if we want to apply special flags, we need to compile again.

check_PROGRAMS= tern_alike_unit_test
TESTS = tern_alike_unit_test

export INCLUDES += -I$(abs_srcdir)/../src/
