MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

#Copy pipeline files required by pipeline tests 
BUILT_SOURCES = pipe_sources
CLEANFILES = pipe_sources
pipe_sources:
	cp -rf $(top_srcdir)/src/rofl/datapath/pipeline/ .

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
	../../memory.c \
	../../empty_packet.c\
	../../platform_empty_hooks_of12.c\
	../../pthread_atomic_operations.c\
	../../pthread_lock.c \
	../../timing.c

unit_test_SOURCES= $(SHARED_SRC)\
			l2hash.c \
			unit_test.c

unit_test_LDADD=$(top_builddir)/src/rofl/librofl_datapath.la -lcunit -lpthread

check_PROGRAMS= unit_test
TESTS = unit_test
