MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

monitoring_unit_test_SOURCES= monitoring_test.c \
			../openflow/openflow1x/pipeline/pthread_lock.c\
			../openflow/openflow1x/pipeline/timing.c\
			../openflow/openflow1x/pipeline/pthread_atomic_operations.c\
			../openflow/openflow1x/pipeline/platform_empty_hooks_of12.c\
			../openflow/openflow1x/pipeline/output_actions.c\
			../openflow/openflow1x/pipeline/memory.c\
			../openflow/openflow1x/pipeline/empty_packet.c

monitoring_unit_test_LDADD= $(top_builddir)/src/rofl/datapath/pipeline/librofl_pipeline.la \
								-lcunit \
								-lpthread
check_PROGRAMS= monitoring_unit_test
TESTS = monitoring_unit_test
