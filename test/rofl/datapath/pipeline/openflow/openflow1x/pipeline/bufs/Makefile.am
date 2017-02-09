MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

SHARED_SRC= ../memory.c \
	../platform_empty_hooks_of12.c\
	../pthread_atomic_operations.c\
	../pthread_lock.c \
	../timing.c

unit_test_SOURCES= $(SHARED_SRC)\
			packet.c\
			test_bufs.c \
			unit_test.c

unit_test_LDADD=$(top_builddir)/src/rofl/datapath/pipeline/librofl_pipeline.la -lcunit -lpthread

check_PROGRAMS= unit_test
TESTS = unit_test
