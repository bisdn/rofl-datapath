#ifndef __OF1X_TRIE_MATCH_H__
#define __OF1X_TRIE_MATCH_H__

#include "rofl_datapath.h"
#include "../matching_algorithms.h"
#include "../../of1x_flow_table.h"


//Data structures
typedef struct of1x_trie_leaf{
	//Match
	of1x_match_t match;

	//Flow entry/ies (ordered by priority)
	of1x_flow_entry_t* entry;

	//Inner max priority
	uint32_t imp;

	//Inner branch(es)
	struct of1x_trie_leaf* inner;

	//Prev
	struct of1x_trie_leaf* prev;

	//Next
	struct of1x_trie_leaf* next;

	//Parent
	struct of1x_trie_leaf* parent;
}of1x_trie_leaf_t;

typedef struct of1x_trie{
	//Root leaf
	of1x_trie_leaf_t* root;

	//Entries with no matches (ordered by priority)
	of1x_flow_entry_t* entry;
}of1x_trie_t;

//C++ extern C
ROFL_BEGIN_DECLS

//Nothing needed here

//C++ extern C
ROFL_END_DECLS

#endif //TRIE_MATCH
