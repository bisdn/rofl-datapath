MAINTAINERCLEANFILES = Makefile.in

.NOTPARALLEL:

# build this before anything else
BUILT_SOURCES = available_ma.h available_ma_pp.h available_ma.c

#####################################
#    Matching algorithms section    # 
#####################################

#Add here your new matching algorithm lib if they need to be compiled by this makefile
EXTRA_LTLIBRARIES = \
	librofl_pipeline_openflow1x_pipeline_matching_algorithms_l2hash.la\
	librofl_pipeline_openflow1x_pipeline_matching_algorithms_loop.la\
	librofl_pipeline_openflow1x_pipeline_matching_algorithms_trie.la

#trie
librofl_pipeline_openflow1x_pipeline_matching_algorithms_trie_ladir = \
	$(library_includedir)/trie

librofl_pipeline_openflow1x_pipeline_matching_algorithms_trie_la_HEADERS = \
	trie/of1x_trie_ma.h\
	trie/of1x_trie_ma_pp.h
librofl_pipeline_openflow1x_pipeline_matching_algorithms_trie_la_SOURCES = \
	trie/of1x_trie_ma.c \
	trie/of1x_trie_ma.h


#l2hash
librofl_pipeline_openflow1x_pipeline_matching_algorithms_l2hash_ladir = \
	$(library_includedir)/l2hash

librofl_pipeline_openflow1x_pipeline_matching_algorithms_l2hash_la_HEADERS = \
	l2hash/of1x_l2hash_ma.h\
	l2hash/of1x_l2hash_ma_pp.h
librofl_pipeline_openflow1x_pipeline_matching_algorithms_l2hash_la_SOURCES = \
	l2hash/of1x_l2hash_ma.c \
	l2hash/of1x_l2hash_ma.h


#Loop

librofl_pipeline_openflow1x_pipeline_matching_algorithms_loop_ladir = \
	$(library_includedir)/loop


librofl_pipeline_openflow1x_pipeline_matching_algorithms_loop_la_HEADERS = \
	loop/of1x_loop_ma.h\
	loop/of1x_loop_ma_pp.h
librofl_pipeline_openflow1x_pipeline_matching_algorithms_loop_la_SOURCES = \
	loop/of1x_loop_ma.c \
	loop/of1x_loop_ma.h


#[+] Add your own here

######################################
# End of Matching algorithms section #
#                                    #
#  Do NOT modify beyoned this point  #
######################################

noinst_LTLIBRARIES = $(MATCHING_ALGORITHM_LIBADD) librofl_pipeline_openflow1x_pipeline_matching_algorithms.la

library_includedir = $(includedir)/rofl/datapath/pipeline/openflow/openflow1x/pipeline/matching_algorithms

library_include_HEADERS = matching_algorithms.h \
	available_ma.h\
	available_ma_pp.h

librofl_pipeline_openflow1x_pipeline_matching_algorithms_la_SOURCES = \
	matching_algorithms.h

nodist_librofl_pipeline_openflow1x_pipeline_matching_algorithms_la_SOURCES = \
	available_ma.c \
	available_ma.h

librofl_pipeline_openflow1x_pipeline_matching_algorithms_la_LIBADD = \
	$(MATCHING_ALGORITHM_LIBADD) \
	$(MATCHING_ALGORITHM_LIBS)

librofl_pipeline_openflow1x_pipeline_matching_algorithms_la_DEPENDENCIES = \
	$(MATCHING_ALGORITHM_LIBADD)

available_ma_pp.h: available_ma_pp.sh Makefile
	$(SHELL) $(srcdir)/available_ma_pp.sh $(srcdir) $(MATCHING_ALGORITHMS) > $(srcdir)/available_ma_pp.h

available_ma.h: available_ma.sh Makefile
	$(SHELL) $(srcdir)/available_ma.sh $(MATCHING_ALGORITHMS) > $(srcdir)/available_ma.h

available_ma.c: available_ma_c.sh Makefile available_ma.h matching_algorithms.h
	$(SHELL) $(srcdir)/available_ma_c.sh $(srcdir) $(MATCHING_ALGORITHMS) > $(srcdir)/available_ma.c

CLEANFILES = $(srcdir)/available_ma.c \
	$(srcdir)/available_ma.h

