MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

dynamic_unit_test_CFLAGS= -DTIMERS_FAKE_TIME  -DOF1X_TIMER_DYNAMIC_ALLOCATION_SLOTS
dynamic_unit_test_CPPFLAGS= -I$(top_srcdir)/src/

#FIXME add output actions test
dynamic_unit_test_SOURCES=../unit_test.c \
	../group_table.c \
	../timers_hard_timeout.c \
	../pp_isolation.c\
	../empty_packet.c \
	../platform_empty_hooks_of12.c\
	../memory.c \
	../pthread_atomic_operations.c \
	../pthread_lock.c \
	../timing.c \
	../lib_random.c

dynamic_unit_test_LDADD=$(top_builddir)/src/rofl/datapath/pipeline/librofl_pipeline.la -lcunit -lpthread

#-lrofl_pipeline -lrofl
# we need to link to the local libraries, not the installed ones
# and if we want to apply special flags, we need to compile again.

check_PROGRAMS= dynamic_unit_test
TESTS = dynamic_unit_test
