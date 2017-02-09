MAINTAINERCLEANFILES = Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

#Copy pipeline files required by pipeline tests 
BUILT_SOURCES = pipe_sources
CLEANFILES = pipe_sources
pipe_sources:
	cp -rf $(top_srcdir)/src/rofl/datapath/pipeline/ .

static_unit_test_CFLAGS= -DTIMERS_FAKE_TIME 
static_unit_test_CPPFLAGS= -I$(top_srcdir)/src/ -DROFL_TEST=1

#FIXME add group table tests!
static_unit_test_SOURCES=../unit_test.c \
	../output_actions.c \
	../timers_hard_timeout.c \
	../pp_isolation.c\
	../empty_packet.c \
	../platform_empty_hooks_of12.c\
	../memory.c \
	../pthread_atomic_operations.c \
	../pthread_lock.c \
	../timing.c \
	../lib_random.c \
	pipeline/physical_switch.c \
	pipeline/monitoring.c \
	pipeline/switch_port.c \
	pipeline/port_queue.c \
	pipeline/openflow/of_switch.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/available_ma.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/loop/of1x_loop_ma.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/l2hash/of1x_l2hash_ma.c \
	pipeline/openflow/openflow1x/pipeline/matching_algorithms/trie/of1x_trie_ma.c \
	pipeline/openflow/openflow1x/of1x_switch.c \
	pipeline/openflow/openflow1x/pipeline/of1x_action.c \
	pipeline/openflow/openflow1x/pipeline/of1x_flow_entry.c \
	pipeline/openflow/openflow1x/pipeline/of1x_flow_table.c \
	pipeline/openflow/openflow1x/pipeline/of1x_group_table.c \
	pipeline/openflow/openflow1x/pipeline/of1x_instruction.c \
	pipeline/openflow/openflow1x/pipeline/of1x_match.c \
	pipeline/openflow/openflow1x/pipeline/of1x_pipeline.c \
	pipeline/openflow/openflow1x/pipeline/of1x_statistics.c \
	pipeline/openflow/openflow1x/pipeline/of1x_timers.c \
	pipeline/common/ternary_fields.c \
	pipeline/common/packet_matches.c \
	pipeline/common/alike_masks.c \
	pipeline/util/logging.c

static_unit_test_LDADD=$(top_builddir)/src/rofl/datapath/pipeline/librofl_pipeline.la -lcunit -lpthread

#-lrofl_pipeline -lrofl
# we need to link to the local libraries, not the installed ones
# and if we want to apply special flags, we need to compile again.

check_PROGRAMS= static_unit_test
TESTS = static_unit_test
