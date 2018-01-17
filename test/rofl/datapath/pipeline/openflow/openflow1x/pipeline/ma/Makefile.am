MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

#Copy pipeline files required by pipeline tests 
BUILT_SOURCES = pipe_sources
CLEANFILES = pipe_sources
pipe_sources:
	cp -rf $(top_srcdir)/src/rofl/datapath/pipeline/ .

SUBDIRS=loop l2hash trie

SHARED_SRC= pipeline/physical_switch.c \
	pipeline/monitoring.c \
	pipeline/switch_port.c \
	pipeline/port_queue.c \
	pipeline/util/logging.c \
	pipeline/common/ternary_fields.c \
	pipeline/common/packet_matches.c \
	pipeline/openflow/of_switch.c \
	pipeline/openflow/openflow1x/of1x_switch.c \
	pipeline/openflow/openflow1x/pipeline/of1x_action.c \
	pipeline/openflow/openflow1x/pipeline/of1x_match.c \
	pipeline/openflow/openflow1x/pipeline/of1x_instruction.c \
	pipeline/openflow/openflow1x/pipeline/of1x_flow_entry.c \
	pipeline/openflow/openflow1x/pipeline/of1x_flow_table.c \
	pipeline/openflow/openflow1x/pipeline/of1x_pipeline.c \
	pipeline/openflow/openflow1x/pipeline/flavors/of1x_pipeline_generic.c \
	pipeline/openflow/openflow1x/pipeline/flavors/of1x_pipeline_ofdpa.c \
	pipeline/openflow/openflow1x/pipeline/of1x_timers.c \
	pipeline/openflow/openflow1x/pipeline/of1x_statistics.c \
	pipeline/openflow/openflow1x/pipeline/of1x_group_table.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/loop/of1x_loop_ma.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/l2hash/of1x_l2hash_ma.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/trie/of1x_trie_ma.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/available_ma.c \
	pipeline/common/alike_masks.c \
	../memory.c \
	../platform_empty_hooks_of12.c\
	../pthread_atomic_operations.c\
	../pthread_lock.c \
	../timing.c

pipe_profile_SOURCES= $(SHARED_SRC)\
			../empty_packet.c\
			profiling_tests.c
pipe_profile_LDADD=$(top_builddir)/src/rofl/librofl_datapath.la -lcunit -lpthread
pipe_profile_CFLAGS = -O3 -pg -fno-omit-frame-pointer
pipe_profile_CXXFLAGS = -O3 -pg -fno-omit-frame-pointer
pipe_profile_CPPFLAGS = -UDEBUG 


lockless_SOURCES= $(SHARED_SRC)\
			../empty_packet.c\
			lockless_tests.c
lockless_LDADD=$(top_builddir)/src/rofl/librofl_datapath.la -lcunit -lpthread
lockless_CFLAGS = -O3 -pg -fno-omit-frame-pointer
lockless_CXXFLAGS = -O3 -pg -fno-omit-frame-pointer
lockless_CPPFLAGS = -UDEBUG 


check_PROGRAMS = lockless pipe_profile
TESTS = lockless pipe_profile 
