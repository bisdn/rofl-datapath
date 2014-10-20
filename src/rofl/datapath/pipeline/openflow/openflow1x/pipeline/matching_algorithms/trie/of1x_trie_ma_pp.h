#ifndef __OF1X_TRIE_MATCH_PP_H__
#define __OF1X_TRIE_MATCH_PP_H__

#include "rofl_datapath.h"
#include "../../of1x_pipeline.h"
#include "../../of1x_flow_table.h"
#include "../../of1x_flow_entry.h"
#include "../../of1x_match_pp.h"
#include "../../of1x_group_table.h"
#include "../../of1x_instruction_pp.h"
#include "../../../of1x_async_events_hooks.h"
#include "../../../../../platform/lock.h"
#include "../../../../../platform/likely.h"
#include "../../../../../platform/memory.h"
#include "of1x_trie_ma.h"

//C++ extern C
ROFL_BEGIN_DECLS

//#define USE_NON_RECURSIVE

#ifndef USE_NON_RECURSIVE

// NOTE: this can never be inlined. Just putting it here
static inline void of1x_check_leaf_trie(datapacket_t *const pkt,
					struct of1x_trie_leaf* leaf,
					int64_t* match_priority,
					of1x_flow_entry_t** best_match){
	if(unlikely(leaf == NULL))
		return;

	//Check inner
	if(((int64_t)leaf->imp) > *match_priority){
		//Check match
		if(__of1x_check_match(pkt, &leaf->match)){
			if(leaf->entry)
				*best_match = leaf->entry;
			if(leaf->inner)
				of1x_check_leaf_trie(pkt,
							leaf->inner,
							match_priority,
							best_match);
		}
	}

	if(leaf->next)
		of1x_check_leaf_trie(pkt, leaf->next,
							match_priority,
							best_match);
}

#else

static inline void of1x_check_leaf_trie(datapacket_t *const pkt,
					struct of1x_trie_leaf* leaf,
					int64_t* match_priority,
					of1x_flow_entry_t** best_match){
	if(unlikely(leaf == NULL))
		return;

CHECK_LEAF:
	//Check inner
	if(((int64_t)leaf->imp) > *match_priority){
		//Check match
		if(__of1x_check_match(pkt, &leaf->match)){
			if(leaf->entry)
				*best_match = leaf->entry;
			if(leaf->inner){
				leaf = leaf->inner;
				goto CHECK_LEAF;
			}
		}
	}

	if(leaf->next){
		leaf = leaf->next;
		goto CHECK_LEAF;
	}else{
		do{
			leaf = leaf->parent;
			if(leaf && leaf->next){
				leaf = leaf->next;
				goto CHECK_LEAF;
			}
		}while(leaf);
	}
}

#endif //!USE_NON_RECURSIVE

/* FLOW entry lookup entry point */
static inline of1x_flow_entry_t* of1x_find_best_match_trie_ma(of1x_flow_table_t *const table,
							datapacket_t *const pkt){

	struct of1x_trie* trie = ((of1x_trie_t*)table->matching_aux[0]);
	int64_t match_priority;
	of1x_flow_entry_t* best_match;
	struct of1x_trie_leaf* leaf = trie->root;

#ifndef ROFL_PIPELINE_LOCKLESS
	//Prevent writers to change structure during matching
	platform_rwlock_rdlock(table->rwlock);
#endif //!ROFL_PIPELINE_LOCKLESS

	//Entries with no matches
	best_match = trie->entry;

	if(best_match)
		match_priority = best_match->priority;
	else
		match_priority = -1;

	//Start recursion
	of1x_check_leaf_trie(pkt, leaf, &match_priority, &best_match);

#ifndef ROFL_PIPELINE_LOCKLESS
	if(best_match){
		//Lock writers to modify the entry while packet processing. WARNING!!!! this must be released by the pipeline, once packet is processed!
		platform_rwlock_rdlock(best_match->rwlock);
	}
#endif //!ROFL_PIPELINE_LOCKLESS

#ifndef ROFL_PIPELINE_LOCKLESS
	//No match
	//Green light for writers
	platform_rwlock_rdunlock(table->rwlock);
#endif //!ROFL_PIPELINE_LOCKLESS

	return best_match;
}

//C++ extern C
ROFL_END_DECLS

#endif //OF1X_TRIE_MATCH_PP
