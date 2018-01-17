#include "of1x_trie_ma.h"

#include <stdlib.h>
#include <assert.h>
#include "../../of1x_pipeline.h"
#include "../../of1x_flow_table.h"
#include "../../of1x_flow_entry.h"
#include "../../of1x_match.h"
#include "../../of1x_group_table.h"
#include "../../of1x_instruction.h"
#include "../../../of1x_async_events_hooks.h"
#include "../../../../../platform/lock.h"
#include "../../../../../platform/likely.h"
#include "../../../../../platform/memory.h"
#include "../../../../../util/logging.h"
#include "../matching_algorithms.h"

#define TRIE_DESCRIPTION "Trie algorithm performs the lookup using a patricia trie"

//
// Constructors and destructors
//
rofl_result_t of1x_init_trie(struct of1x_flow_table *const table){

	//Allocate main trie struct
	table->matching_aux[0] = (void*)platform_malloc_shared(sizeof(struct of1x_trie));
	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];

	//Set values
	trie->entry = NULL;
	trie->root = NULL;

	return ROFL_SUCCESS;
}

//Recursively destroy leafs
static void of1x_destroy_leaf(struct of1x_trie_leaf* leaf){

	of1x_flow_entry_t *entry, *tmp;

	if(!leaf)
		return;

	//Destroy entry/ies
	entry = leaf->entry;
	while(entry){
		tmp = entry->next;
		of1x_destroy_flow_entry(entry);
		entry = tmp;
	}

	//Destroy inner leafs
	of1x_destroy_leaf(leaf->inner);

	//Destroy next one(s)
	of1x_destroy_leaf(leaf->next);

	//Free our memory
	platform_free_shared(leaf);
}

rofl_result_t of1x_destroy_trie(struct of1x_flow_table *const table){

	of1x_flow_entry_t *entry, *tmp;
	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];

	//Free all the leafs
	of1x_destroy_leaf(trie->root);

	//Destroy entry/ies
	entry = trie->entry;
	while(entry){
		tmp = entry->next;
		of1x_destroy_flow_entry(entry);
		entry = tmp;
	}

	//Free main leaf structure
	platform_free_shared(table->matching_aux[0]);

	return ROFL_FAILURE;
}

//
//Linked-list
//
void __of1x_add_ll_prio_trie(of1x_flow_entry_t** head,
					of1x_flow_entry_t* entry){

	of1x_flow_entry_t *tmp;

	if(!head){
		assert(0);
		return;
	}

	//No head
	if(*head == NULL){
		entry->prev = entry->next = NULL;
		*head = entry;
		return;
	}

	//Loop over the entries and find the right position
	tmp = *head;
	while(tmp){
		//Insert before tmp
		if(tmp->priority < entry->priority){
			entry->prev = tmp->prev;
			entry->next = tmp;
			if(tmp->prev)
				tmp->prev->next = entry;
			else
				*head = entry;
			tmp->prev = entry;
			return;
		}
		//If it is the last one insert in the tail
		//and return
		if(!tmp->next){
			tmp->next = entry;
			entry->prev = tmp;
			entry->next = NULL;
			return;
		}
		tmp = tmp->next;
	}

	//Cannot reach this point
	assert(0);
}

void __of1x_remove_ll_prio_trie(of1x_flow_entry_t** head,
						of1x_flow_entry_t* entry){

	if(!head || *head == NULL){
		assert(0);
		return;
	}

	if(entry == *head){
		//It is the head
		*head = entry->next;
		if(entry->next)
			entry->next->prev = NULL;
	}else{
		//This is used during matching
		entry->prev->next = entry->next;

		//This is not
		if(entry->next)
			entry->next->prev = entry->prev;
	}
}

//
//Helper functions
//
static bool __of1x_is_tern_submatch_trie(of1x_match_t* match,
							of1x_match_group_t *const matches,
							bool wildcard_matches){
	of1x_match_t* sub_match;

	if(!match)
		return wildcard_matches;

 	sub_match = matches->m_array[match->type];

	if(!sub_match)
		return wildcard_matches;

	return __of1x_is_submatch(match, sub_match);
}

static bool __of1x_is_tern_complete_match_trie(of1x_match_t* match,
							of1x_match_group_t *const matches,
							bool wildcard_matches){
	of1x_match_t* m = matches->m_array[match->type];
	of1x_match_t alike_match;

	if(!m || !match)
		return wildcard_matches;

	if(!__of1x_get_alike_match(match, m, &alike_match))
		return false;
	return __of1x_equal_matches(match, &alike_match);
}

//
// Utils
//

//
// Find matching leaf branches/entries.
//
// This is a swiss-knife function
// that can match or perform lookups, depending on the flags
//
//
// @param matches Match agains this
// @param prev Last leaf matched
// @param next Next leaf to continue the search
// @param all Check all entries (follow all paths)
// @param lookup Perform find using lookup criteria; full wildcard
// (no match) is a positive match.
//
static of1x_flow_entry_t* of1x_find_reen_trie( of1x_match_group_t *const matches,
								struct of1x_trie_leaf** prev,
								struct of1x_trie_leaf** next,
								bool all,
								bool check_complete,
								bool lookup){
	of1x_flow_entry_t* res = NULL;
	of1x_match_t* leaf_match;
	of1x_trie_leaf_t* curr;

	assert((all&lookup) == false);

FIND_START:
	//Set next
	curr = *next;

	//Sanity checks
	assert(!curr || !curr->prev || (curr->prev->next == curr));
	assert(!curr || !curr->next || (curr->next->prev == curr));
	assert(!curr || !curr->parent || curr->prev || (curr->parent->inner == curr));

	//If next is NULL and the prev has a parent, that means go down
	//in the tree. It might be that the very last leaf of the tree is matched
	//hence we are done
	if(!curr){
		if(*prev && (*prev)->parent){
			//No next and parent => move back to parent and
			//process next
			curr = (*prev)->parent;
			goto FIND_NEXT;
		}else{
			goto FIND_END; //No more entries in the trie
		}
	}

	//Check itself
	leaf_match = &curr->match;
	if(!all && !__of1x_is_tern_submatch_trie(leaf_match, matches, lookup))
		goto FIND_NEXT;

	//Matches, check if there is an entry

	if(curr->entry){
		if(!check_complete ||
			__of1x_is_tern_complete_match_trie(leaf_match,
								matches,
								lookup) == true){
			//Set result
			res = curr->entry;

			//Set prev and next pointers
			(*prev) = curr;
			if(curr->inner)
				*next = curr->inner;
			else
				*next = curr->next;
			goto FIND_END;
		}
	}

	//Check inner leafs
	if(curr->inner){
		if(check_complete){
			//If check_complete is 1, we have to make sure
			//we match completely and not partially the match
			if(leaf_match->type != curr->inner->match.type &&
				__of1x_is_tern_complete_match_trie(leaf_match,
								matches,
								lookup) == false)
				goto FIND_NEXT;
		}
		//Check inner
		(*prev) = curr;
		*next = curr->inner;

		goto FIND_START;
	}

FIND_NEXT:
	(*prev) = curr;
	*next = curr->next;
	goto FIND_START;

FIND_END:
	return res;
}


static bool __of1x_check_priority_cookie_trie(of1x_flow_entry_t *const entry,
						of1x_flow_entry_t *const trie_entry,
						bool check_priority,
						bool check_cookie){
	//Check cookie first
	if(check_cookie && entry->cookie != OF1X_DO_NOT_CHECK_COOKIE && entry->cookie_mask){
		if( (entry->cookie & entry->cookie_mask) != (trie_entry->cookie & entry->cookie_mask) )
			return false;
	}

	//Check priority
	if(check_priority && ((entry->priority&OF1X_2_BYTE_MASK) != (trie_entry->priority&OF1X_2_BYTE_MASK)))
		return false;

	return true;
}

static inline int __of1x_get_next_match(of1x_flow_entry_t *const entry, int curr){
	int i;
	for(i = (curr < 0)? 0 : (curr+1); i< OF1X_MATCH_MAX; i++)
		if(entry->matches.m_array[i])
			return i;
	return -1;
}

//
//Leaf mgmt
//

static of1x_trie_leaf_t*  __of1x_create_new_branch_trie(int m_it, of1x_flow_entry_t *const entry){

	of1x_trie_leaf_t *new_branch = NULL;
	of1x_trie_leaf_t *last_leaf = NULL;
	of1x_trie_leaf_t *tmp;
	of1x_match_t* m;

	while(m_it != -1){
		m = entry->matches.m_array[m_it];

		//Allocate space
		tmp = (of1x_trie_leaf_t*)platform_malloc_shared(sizeof(of1x_trie_leaf_t));
		if(!tmp){
			//Out of memory
			assert(0);
			return NULL;
		}

		//Init
		memset(tmp, 0, sizeof(of1x_trie_leaf_t));

		//Copy match (cannot fail)
		if(!__of1x_get_alike_match(m, m, &tmp->match)){
			assert(0);
			return NULL;
		}

		//Fill-in
		if(last_leaf){
			last_leaf->entry = NULL;
			last_leaf->next = last_leaf->prev = NULL;
			last_leaf->imp = entry->priority;
		}

		//Set the linked list
		if(!last_leaf){
			new_branch = last_leaf = tmp;
		}else{
			last_leaf->inner = tmp;
			tmp->parent = last_leaf;
			last_leaf = tmp;
		}
		m_it = __of1x_get_next_match(entry, m_it);
	}

	last_leaf->entry = entry;
	last_leaf->imp = entry->priority;

	return new_branch;
}


//Append to the "next"
static rofl_of1x_fm_result_t __of1x_insert_next_leaf_trie(struct of1x_trie_leaf* l,
							int m_it,
							of1x_flow_entry_t *const entry){

	struct of1x_trie_leaf* new_branch;
	struct of1x_trie_leaf* l_poi;

	//Get the new branch
	new_branch = __of1x_create_new_branch_trie(m_it, entry);

	if(!new_branch)
		return ROFL_OF1X_FM_FAILURE;

	//Fill-in missing information
	new_branch->parent = l->parent;
	new_branch->prev = l;

	/*
	* Adjust inner max priority
	*/

	//Then go recursively down
	l_poi = l;
	l = l->parent;
	while(l){
		if(l->imp < entry->priority)
			l->imp = entry->priority;
		else
			break;
		l = l->parent;
	}

	//We append as next
	entry->prev = entry->next = NULL;
	l_poi->next = new_branch;

	return ROFL_OF1X_FM_SUCCESS;
}

//Add an intermediate match and branch from that point
static rofl_of1x_fm_result_t __of1x_insert_intermediate_leaf_trie(of1x_trie_t* trie,
							struct of1x_trie_leaf* l,
							int m_it,
							of1x_flow_entry_t *const entry){
	of1x_trie_leaf_t *intermediate;
	struct of1x_trie_leaf* new_branch=NULL;
	of1x_match_t* m;

	m = entry->matches.m_array[m_it];

	//Allocate space
	intermediate = (of1x_trie_leaf_t*)platform_malloc_shared(sizeof(of1x_trie_leaf_t));
	if(!intermediate){
		//Out of memory
		assert(0);
		return ROFL_OF1X_FM_FAILURE;
	}
	memset(intermediate, 0, sizeof(of1x_trie_leaf_t));

	//Get the common part
	if(!__of1x_get_alike_match(&l->match, m, &intermediate->match)){
		//Can never (ever) happen
		assert(0);
		return ROFL_OF1X_FM_FAILURE;
	}

	//If they are equal, get the next match
	if(__of1x_equal_matches(m, &intermediate->match) == true)
		m_it = __of1x_get_next_match(entry, m_it);

	//Get the new branch if there are other matches
	if(m_it != -1){
		new_branch = __of1x_create_new_branch_trie(m_it, entry);
		if(!new_branch)
			return ROFL_OF1X_FM_FAILURE;
	}else{
		intermediate->entry = entry;
	}


	/*
	* Fill in the intermediate and new branch
	*/

	//Intermediate leaf
	intermediate->inner = l;
	intermediate->parent = l->parent;
	intermediate->prev = l->prev;
	intermediate->next = l->next;

	//new branch
	if(new_branch){
		new_branch->parent = intermediate;
		new_branch->prev = l;
		new_branch->next = NULL;
	}

	if(l->next)
		l->next->prev = intermediate;
	if(l->prev)
		l->prev->next = intermediate;

	//Previous leaf now child
	l->parent = intermediate;
	l->next = new_branch;
	l->prev = NULL;

	//Entry linked list
	entry->prev = entry->next = NULL;

	/*
	* Adjust imp
	*/
	if(l->imp < entry->priority){
		//Adjust intermediate
		intermediate->imp = entry->priority;

		//Recursively adjust
		l = intermediate->parent;
		while(l){
			if(l->imp < entry->priority)
				l->imp = entry->priority;
			else
				break;
			l = l->parent;
		}
	}else{
		intermediate->imp = l->imp;
	}

	//Now insert
	if(intermediate->prev){
			intermediate->prev->next = intermediate;
	}else{
		if(intermediate->parent)
			intermediate->parent->inner = intermediate;
		else
			trie->root = intermediate;
	}
	return ROFL_OF1X_FM_SUCCESS;
}

static rofl_of1x_fm_result_t __of1x_insert_terminal_leaf_trie(struct of1x_trie_leaf* l,
							int m_it,
							of1x_flow_entry_t *const entry){
	uint32_t max_priority = entry->priority;

	//l is the terminal leaf (entry has no more matches)
	if(l->entry){
		//Append to the very last in the linked list
		__of1x_add_ll_prio_trie(&l->entry, entry);
	}else{
		l->entry = entry;
		entry->prev = NULL;
		entry->next = NULL;
	}

	/*
	* Adjust imp
	*/
	max_priority = (max_priority > l->imp)? max_priority : l->imp;
	while(l){
		if(l->imp < max_priority)
			l->imp = max_priority;
		else
			break;
		l = l->parent;
	}

	return ROFL_OF1X_FM_SUCCESS;
}

//Perform the real addition to the trie
rofl_of1x_fm_result_t __of1x_add_leafs_trie(of1x_trie_t* trie,
							of1x_flow_entry_t *const entry){

	int m_it;
	struct of1x_trie_leaf *l;
	of1x_match_t *m, tmp;
	rofl_of1x_fm_result_t res = ROFL_OF1X_FM_SUCCESS;

	//Determine entry's first match
	m_it = __of1x_get_next_match(entry, -1);

	if(m_it == -1){
		//This is an empty entry, therefore it needs to be
		//added in the root of the tree
		__of1x_add_ll_prio_trie(&trie->entry, entry);
		goto ADD_LEAFS_END;
	}

	//Start by the very first leaf
	l = trie->root;
	m = entry->matches.m_array[m_it];

	if(!l){
		//This is the point of insertion
		trie->root = __of1x_create_new_branch_trie(m_it, entry);
		if(!trie->root)
			res = ROFL_OF1X_FM_FAILURE;
		goto ADD_LEAFS_END;
	}

	//Determine the point of insertion
	while(l){
		//Check if they share something
		if(__of1x_get_alike_match(m, &l->match, &tmp) == false)
			goto ADD_LEAFS_NEXT;

		//
		// They do share something
		// but is it already what parent was sharing
		//
		if(l->parent && __of1x_equal_matches(&l->parent->match, &tmp))
			goto ADD_LEAFS_NEXT;

		//If what they sahre is equal to the leaf match
		//then we have to go deeper
		if(__of1x_equal_matches(&l->match, &tmp)){
			//Check if the they m == leaf
			if(__of1x_equal_matches(&l->match, m) == false)
				goto ADD_LEAFS_INNER;

			//Increment match
			if(__of1x_get_next_match(entry, m_it) == -1){
				//No more matches; this is the insertion point
				res = __of1x_insert_terminal_leaf_trie(l, m_it, entry);
				goto ADD_LEAFS_END;
			}

			//Go to next match only if there are inner branches
			if(l->inner){
				m_it = __of1x_get_next_match(entry, m_it);
				m = entry->matches.m_array[m_it];
			}

			//Go deeper
			goto ADD_LEAFS_INNER;
		}else{
			//leaf an m share something; this is the point of insertion
			res = __of1x_insert_intermediate_leaf_trie(trie, l, m_it,
										entry);
			goto ADD_LEAFS_END;
		}

ADD_LEAFS_INNER:
		if(l->inner){
			l = l->inner;
			continue;
		}

		//This is the point of insertion
		assert(m_it == l->match.type);
		res = __of1x_insert_intermediate_leaf_trie(trie, l, m_it,
								entry);
		goto ADD_LEAFS_END;
ADD_LEAFS_NEXT:
		if(!l->next){
			//This is the point of insertion
			res = __of1x_insert_next_leaf_trie(l, m_it, entry);
			goto ADD_LEAFS_END;
		}
		l = l->next;
	}

	//We shall never reach this point
	assert(0);
	return ROFL_OF1X_FM_FAILURE;

ADD_LEAFS_END:

	return res;
}

bool __of1x_prune_leafs_trie(of1x_flow_table_t *const table, of1x_trie_t* trie,
							of1x_trie_leaf_t* prev){

	//Make code readable
	of1x_trie_leaf_t *to_prune=NULL, *aux;
	of1x_flow_entry_t *it;
	int64_t max;

	//no-match entry
	if(!prev)
		return false;

	//If we have more entries or we are an intermediate
	//leaf, just return
	if(prev->entry || prev->inner)
		return false;

	//Check how many leafs do not have children
	//(poor them)
	aux = prev;
	while(1){
		if(aux->entry) break;
		to_prune = aux;
		if(!aux->parent || aux->prev || aux->next) break;
		aux = aux->parent;
	}

	if(!to_prune){
		assert(0);
		return false;
	}

	//Remove from the linked list
	if(to_prune->prev){
		to_prune->prev->next = to_prune->next;
		if(to_prune->next)
			to_prune->next->prev = to_prune->prev;
	}else{
		if(to_prune->parent)
			to_prune->parent->inner = to_prune->next;
		else
			trie->root = to_prune->next;
		if(to_prune->next)
			to_prune->next->prev = NULL;
	}

	//Adjust max inner priority (downgrade, eventually)
	aux = to_prune->parent;
	while(aux){
		//If we were not the highest priority
		//stop
		max = 0;
		it = aux->entry;
		while(it){
			max = (it->priority > max) ? it->priority : max;
			it = it->next;
		}

		if(aux->inner && aux->imp > max)
			max = aux->imp;

		if(aux->imp == max)
			break;

		aux->imp = max;
		aux = aux->parent;
	}

	//Make sure we don't have any thread processing packets
#ifdef ROFL_PIPELINE_LOCKLESS
	tid_wait_all_not_present(&table->tid_presence_mask);
#endif

	//Isolate
	to_prune->next = to_prune->prev = NULL;

	//Destroy the entire branch
	of1x_destroy_leaf(to_prune);

	return true;
}



//
// Main routines
//
rofl_of1x_fm_result_t of1x_add_flow_entry_trie(of1x_flow_table_t *const table,
								of1x_flow_entry_t *const entry,
								bool check_overlap,
								bool reset_counts,
								bool check_cookie){
	rofl_of1x_fm_result_t res = ROFL_OF1X_FM_SUCCESS;
	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];
	struct of1x_trie_leaf *prev, *next;
	of1x_flow_entry_t *curr_entry, *to_be_removed=NULL, **ll_head;

	//Allow single add/remove operation over the table
	platform_mutex_lock(table->mutex);

	//Do not allow stats during insertion
	platform_rwlock_wrlock(table->rwlock);

	/*
	* Check overlap
	*/

	if(check_overlap){

		//Empty matches group (all)
		of1x_match_group_t matches;
		__of1x_init_match_group(&matches);

		//Point to the root of the tree
		prev = NULL;
		next = trie->root;

		//No match entries
		curr_entry = trie->entry;

		do{
			//Get next entry (all)
			if(!curr_entry)
				curr_entry = of1x_find_reen_trie(&matches, &prev,
										&next,
										true,
										false,
										false);

			//If no more entries are found, we are done
			if(!curr_entry)
				break;

			//Really check overlap
			if(__of1x_flow_entry_check_overlap(curr_entry, entry, true, false,
										OF1X_PORT_ANY,
										OF1X_GROUP_ANY)){
				res = ROFL_OF1X_FM_OVERLAP;
				ROFL_PIPELINE_ERR("[flowmod-add(%p)][trie] Overlaps with, at least, another entry (%p)\n", entry, curr_entry);
				goto ADD_END;
			}

			curr_entry = curr_entry->next;
		}while(1);
	}

	/*
	* Check exact (overwrite)
	*/

	//Point to the root of the tree
	prev = NULL;
	next = trie->root;

	//No match entries
	if(!entry->matches.head)
		curr_entry = trie->entry;
	else
		curr_entry = NULL;

	do{
		//Get next exact
		if(!curr_entry){
			//Check existent (they can only be in this position of the trie,
			//but there can be multiple ones chained under "next" pointer (different priorities)
			curr_entry = of1x_find_reen_trie(&entry->matches, &prev,
									&next,
									false,
									true,
									false);
			//If no more entries are found, we are done
			if(!curr_entry)
				break;
		}

		if(curr_entry && __of1x_check_priority_cookie_trie(entry, curr_entry,
										true,
										check_cookie)){
			ROFL_PIPELINE_DEBUG("[flowmod-modify(%p)][trie] Existing entry (%p) will be updated with (%p)\n", entry, curr_entry, entry);

			//Head of the entry linked list
			if(prev)
				ll_head = &prev->entry;
			else
				ll_head = &trie->entry;

			//Add the new entry
			__of1x_add_ll_prio_trie(ll_head, entry);

			//Remove the previous
			__of1x_remove_ll_prio_trie(ll_head, curr_entry);

			//Mark the entry to be removed
			to_be_removed = curr_entry;

			//Update stats
			if(!reset_counts){
				__of1x_stats_flow_tid_t c;

#ifdef ROFL_PIPELINE_LOCKLESS
				tid_wait_all_not_present(&table->tid_presence_mask);
#endif

				//Consolidate stats
				__of1x_stats_flow_consolidate(&curr_entry->stats, &c);

				//Add; note that position 0 is locked by us
				curr_entry->stats.s.__internal[0].packet_count += c.packet_count;
				curr_entry->stats.s.__internal[0].byte_count += c.byte_count;
			}

			//Call the platform hook
			platform_of1x_modify_entry_hook(curr_entry, entry, reset_counts);

			goto ADD_END;
		}
		curr_entry = curr_entry->next;
	}while(1);

	//If we got in here, we have to add the entry (no existing entries)
	res = __of1x_add_leafs_trie(trie, entry);

	if(res == ROFL_OF1X_FM_SUCCESS){
		//Call the platform
		plaftorm_of1x_add_entry_hook(entry);
		table->num_of_entries++;
	}

	//Set table pointer
	entry->table = table;

ADD_END:
	platform_rwlock_wrunlock(table->rwlock);
	platform_mutex_unlock(table->mutex);

	if(to_be_removed){
#ifdef ROFL_PIPELINE_LOCKLESS
		tid_wait_all_not_present(&table->tid_presence_mask);
#endif
		of1x_destroy_flow_entry(to_be_removed);
	}

	return res;
}

rofl_of1x_fm_result_t of1x_modify_flow_entry_trie(of1x_flow_table_t *const table,
						of1x_flow_entry_t *const entry,
						const enum of1x_flow_removal_strictness strict,
						bool reset_counts){

	struct of1x_trie_leaf *prev, *next;
	of1x_flow_entry_t *it;
	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];
	rofl_of1x_fm_result_t res = ROFL_OF1X_FM_SUCCESS;
	bool check_cookie = ( table->pipeline->sw->of_ver != OF_VERSION_10 ); //Ignore cookie in OF1.0
	unsigned int moded=0;

	//Allow single add/remove operation over the table
	platform_mutex_lock(table->mutex);

	//Do not allow stats during insertion
	platform_rwlock_wrlock(table->rwlock);

	//Point to the root of the tree
	prev = NULL;
	next = trie->root;

	//No match entries
	if(!entry->matches.head)
		it = trie->entry;
	else
		it = NULL;

	do{
		//Get next matching entry
		if(!it)
			it = of1x_find_reen_trie(&entry->matches, &prev,
									&next,
									false,
									strict == STRICT,
									true);
		//If no more entries are found, we are done
		if(!it)
			goto MODIFY_END;

		//If strict check matches to be strictly the same
		if(strict == STRICT){
			if(__of1x_check_priority_cookie_trie(entry, it, true, check_cookie) == false)
				goto MODIFY_NEXT;

			if(__of1x_flow_entry_check_equal(it, entry, OF1X_PORT_ANY,
									OF1X_GROUP_ANY,
									check_cookie) == false)
			goto MODIFY_NEXT;
		}

		/*
		* If we reach this point we have to modify the current entry
		*/

		//Call platform
		platform_of1x_modify_entry_hook(it, entry, reset_counts);
		ROFL_PIPELINE_DEBUG("[flowmod-modify(%p)] Existing entry (%p) will be updated with (%p)\n", entry, it, entry);
		if(__of1x_update_flow_entry(it, entry, reset_counts) != ROFL_SUCCESS){
			res = ROFL_OF1X_FM_FAILURE;
			goto MODIFY_END;
		}
		moded++;

		//If modification was strict; we are done
		//(there cannot be 2 entries that are equal in the tree)
		if(strict == STRICT)
			goto MODIFY_END;
MODIFY_NEXT:

		it = it->next;
	}while(1);

MODIFY_END:
	platform_rwlock_wrunlock(table->rwlock);
	platform_mutex_unlock(table->mutex);

#ifdef ROFL_PIPELINE_LOCKLESS
	tid_wait_all_not_present(&table->tid_presence_mask);
#endif

	//According to spec
	if(moded == 0 && res == ROFL_OF1X_FM_SUCCESS)
		res = of1x_add_flow_entry_trie(table, entry, false, reset_counts, check_cookie);
	else
		of1x_destroy_flow_entry(entry);

	return res;
}

rofl_of1x_fm_result_t of1x_remove_flow_entry_trie(of1x_flow_table_t *const table,
						of1x_flow_entry_t *const entry,
						of1x_flow_entry_t *const specific_entry,
						const enum of1x_flow_removal_strictness strict,
						uint32_t out_port,
						uint32_t out_group,
						of1x_flow_remove_reason_t reason,
						of1x_mutex_acquisition_required_t mutex_acquired){

	struct of1x_trie_leaf *prev, *next;
	of1x_flow_entry_t *it, *aux, *tmp_next;
	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];

	rofl_of1x_fm_result_t res = ROFL_OF1X_FM_SUCCESS;
	rofl_result_t r; //Auxiliary (destroy)
	bool check_cookie = ( table->pipeline->sw->of_ver != OF_VERSION_10 ); //Ignore cookie in OF1.0

	//Basic sanitychecks
	if( (entry&&specific_entry) ||  (!entry && !specific_entry) )
		return ROFL_OF1X_FM_FAILURE;

	//Allow single add/remove operation over the table
	if(!mutex_acquired)
		platform_mutex_lock(table->mutex);

	//Do not allow stats during insertion
	platform_rwlock_wrlock(table->rwlock);

	//Point to the root of the tree
	prev = NULL;
	next = trie->root;

	//Set aux
	aux = (entry)? entry : specific_entry;

	//No match entries
	if(!aux->matches.head)
		it = trie->entry;
	else
		it = NULL;

	do{
		//Get next matching entry
		if(!it){
			it = of1x_find_reen_trie(&aux->matches, &prev,
									&next,
									false,
									strict == STRICT,
									true);
		}
		//If no more entries are found, we are done
		if(!it)
			goto REMOVE_END;

		//Fast skip
		if(specific_entry && it != specific_entry)
			goto REMOVE_NEXT;

		//Check priority and cookie
		if(strict == STRICT){
			if(__of1x_check_priority_cookie_trie(aux, it, true,
									check_cookie) == false
				|| __of1x_flow_entry_check_equal(it, aux, out_port,
									out_group,
									check_cookie) == false)
				goto REMOVE_NEXT;
		}else{
			if(__of1x_flow_entry_check_contained(it, aux, false,
									check_cookie,
									out_port,
									out_group,
									true) == false)
				goto REMOVE_NEXT;
		}

		/*
		* If we reach this point we have to remove the current entry
		*/
		//Print a nice trace
		__of1x_remove_flow_entry_table_trace(" [trie]", aux, it, reason);

		//Call platform hook
		platform_of1x_remove_entry_hook(it);

		//Save before removal
		tmp_next = it->next;

		//Perform removal
		if(prev)
			__of1x_remove_ll_prio_trie(&prev->entry, it);
		else
			__of1x_remove_ll_prio_trie(&trie->entry, it);

		//Wait for all cores to be aware
#ifdef ROFL_PIPELINE_LOCKLESS
		tid_wait_all_not_present(&table->tid_presence_mask);
#endif
		r = __of1x_destroy_flow_entry_with_reason(it, reason);

		if(r != ROFL_SUCCESS){
			res = ROFL_OF1X_FM_FAILURE;
			goto REMOVE_END;
		}

		//Prune (collapse) trie
		if(__of1x_prune_leafs_trie(table, trie, prev)){
			//If leafs have been destroyed, we
			//need to start from the very beginning
			prev = NULL;
			next = trie->root;
		}

		table->num_of_entries--;

		it = tmp_next;
		continue;
REMOVE_NEXT:

		it = it->next;
	}while(1);


REMOVE_END:
	platform_rwlock_wrunlock(table->rwlock);
	if(!mutex_acquired)
		platform_mutex_unlock(table->mutex);

	return res;
}

rofl_result_t of1x_get_flow_stats_trie(struct of1x_flow_table *const table,
		uint64_t cookie,
		uint64_t cookie_mask,
		uint32_t out_port,
		uint32_t out_group,
		of1x_match_group_t *const matches,
		of1x_stats_flow_msg_t* msg){

	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];
	struct of1x_trie_leaf *prev, *next;
	bool check_cookie;
	of1x_flow_entry_t flow_stats_entry, *it;
	of1x_stats_single_flow_msg_t* flow_stats;
	rofl_result_t res = ROFL_SUCCESS;

	if( unlikely(msg==NULL) || unlikely(table==NULL) )
		return ROFL_FAILURE;

	//Flow stats entry for easy comparison
	platform_memset(&flow_stats_entry, 0, sizeof(of1x_flow_entry_t));
	flow_stats_entry.matches = *matches;
	flow_stats_entry.cookie = cookie;
	flow_stats_entry.cookie_mask = cookie_mask;
	check_cookie = ( table->pipeline->sw->of_ver != OF_VERSION_10 ); //Ignore cookie in OF1.0

	//Mark table as being read
	platform_rwlock_rdlock(table->rwlock);

	//Point to the root of the tree
	prev = NULL;
	next = trie->root;

	//No match entries
	it = trie->entry;

	do{
		//Get next matching entry
		if(!it)
			it = of1x_find_reen_trie(&flow_stats_entry.matches, &prev, &next,
										false,
										false,
										true);
		//If no more entries are found, we are done
		if(!it)
			break;

		//Check if it is really contained and out port/group
		if(__of1x_flow_entry_check_contained(&flow_stats_entry, it, false,
									check_cookie,
									out_port,
									out_group,
									true) == false)
			goto STATS_NEXT;

		//Update statistics from platform
		platform_of1x_update_stats_hook(it);

		//Create a new single flow entry and filling
		flow_stats = __of1x_init_stats_single_flow_msg(it);

		if(!flow_stats){
			res = ROFL_FAILURE;
			goto FLOW_STATS_END;
		}

		//Push this stat to the msg
		__of1x_push_single_flow_stats_to_msg(msg, flow_stats);

STATS_NEXT:
		it = it->next;
	}while(1);

FLOW_STATS_END:

	//Release the table
	platform_rwlock_rdunlock(table->rwlock);

	return res;
}

rofl_result_t of1x_get_flow_aggregate_stats_trie(struct of1x_flow_table *const table,
		uint64_t cookie,
		uint64_t cookie_mask,
		uint32_t out_port,
		uint32_t out_group,
		of1x_match_group_t *const matches,
		of1x_stats_flow_aggregate_msg_t* msg){

	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];
	struct of1x_trie_leaf *prev, *next;
	bool check_cookie;
	of1x_flow_entry_t flow_stats_entry, *it;

	if( unlikely(msg==NULL) || unlikely(table==NULL) )
		return ROFL_FAILURE;

	//Flow stats entry for easy comparison
	platform_memset(&flow_stats_entry, 0, sizeof(of1x_flow_entry_t));
	flow_stats_entry.matches = *matches;
	flow_stats_entry.cookie = cookie;
	flow_stats_entry.cookie_mask = cookie_mask;
	check_cookie = ( table->pipeline->sw->of_ver != OF_VERSION_10 ); //Ignore cookie in OF1.0

	//Mark table as being read
	platform_rwlock_rdlock(table->rwlock);

	//Point to the root of the tree
	prev = NULL;
	next = trie->root;

	//No match entries
	it = trie->entry;

	do{
		__of1x_stats_flow_tid_t c;

		//Get next matching entry
		if(!it)
			it = of1x_find_reen_trie(&flow_stats_entry.matches, &prev, &next,
										false,
										false,
										true);
		//If no more entries are found, we are done
		if(!it)
			break;

		//Check if it is really contained and out port/group
		if(__of1x_flow_entry_check_contained(&flow_stats_entry, it, false,
									check_cookie,
									out_port,
									out_group,
									true) == false)
			goto AGGREGATE_STATS_NEXT;

		// update statistics from platform
		platform_of1x_update_stats_hook(it);

		//Consolidate stats
		__of1x_stats_flow_consolidate(&it->stats, &c);

		msg->packet_count += c.packet_count;
		msg->byte_count += c.byte_count;
		msg->flow_count++;

AGGREGATE_STATS_NEXT:
		it = it->next;
	}while(1);

	//Release the table
	platform_rwlock_rdunlock(table->rwlock);

	return ROFL_SUCCESS;
}

of1x_flow_entry_t* of1x_find_entry_using_group_trie(of1x_flow_table_t *const table,
							const unsigned int group_id){

	struct of1x_trie_leaf *prev, *next;
	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];
	of1x_flow_entry_t *it, *found = NULL;

	//Empty matches group (all)
	of1x_match_group_t matches;
	__of1x_init_match_group(&matches);

	//Prevent writers to change structure during matching
	platform_rwlock_rdlock(table->rwlock);

	//Point to the root of the tree
	prev = NULL;
	next = trie->root;

	//No match entries
	it = trie->entry;

	do{
		//Get next matching entry
		if(!it)
			it = of1x_find_reen_trie(&matches, &prev,
									&next,
									true,
									false,
									false);
		//If no more entries are found, we are done
		if(!it)
			goto FIND_GROUP_END;

		//Check if it really contains group inst
		if(__of1x_instructions_contain_group(it, group_id)){
			found = it;
			break;
		}

		it = it->next;
	}while(1);

FIND_GROUP_END:
	platform_rwlock_rdunlock(table->rwlock);

	return found;
}


#define INDENT "  "

static void of1x_dump_leaf_trie(struct of1x_trie_leaf *l, int indent, bool raw_nbo){

	int i;

	if(!l)
		return;

	assert(!l->parent || l->parent->inner!=l || (l->prev == NULL ));

	ROFL_PIPELINE_INFO("[trie]");

	for(i = indent; i >= 0; i--)
		ROFL_PIPELINE_INFO_NO_PREFIX(INDENT);

	ROFL_PIPELINE_INFO_NO_PREFIX("l:");
	__of1x_dump_matches(&l->match, raw_nbo);
	ROFL_PIPELINE_INFO_NO_PREFIX(" imp: %u ",l->imp);

	if(l->entry)
		ROFL_PIPELINE_INFO_NO_PREFIX("* p: %u (%p)", l->entry->priority, l->entry);
	ROFL_PIPELINE_INFO_NO_PREFIX("\n");

	//Inner
	if(l->inner)
		of1x_dump_leaf_trie(l->inner, indent+1, raw_nbo);

	//Next
	if(l->next)
		of1x_dump_leaf_trie(l->next, indent, raw_nbo);
}

void of1x_dump_trie(struct of1x_flow_table *const table, bool raw_nbo){

	of1x_trie_t* trie = (of1x_trie_t*)table->matching_aux[0];
	of1x_flow_entry_t* it;
	struct of1x_trie_leaf *prev, *next;
	__of1x_stats_table_tid_t c;
	of1x_match_group_t matches;

	__of1x_init_match_group(&matches);
	__of1x_stats_table_consolidate(&table->stats, &c);

	ROFL_PIPELINE_INFO("[trie]\n");
	ROFL_PIPELINE_INFO("[trie] Dumping table # %u (%p). Default action: %s, table-id: %u, next table-id: %u, num. of entries: %d, ma: %u statistics {looked up: %u, matched: %u}\n", table->number, table, __of1x_flow_table_miss_config_str[table->default_action], table->table_index, table->table_index_next, table->num_of_entries, table->matching_algorithm,  c.lookup_count, c.matched_count);
	ROFL_PIPELINE_INFO("[trie]\n");
	ROFL_PIPELINE_INFO("[trie] No matches:\n");

	//Empty entry
	it = trie->entry;
	while(it){
		ROFL_PIPELINE_INFO("[trie]"INDENT" * p: %u (%p)\n", it->priority, it);
		it = it->next;
	}
	if(trie->entry)
		ROFL_PIPELINE_INFO("[trie]\n");
	ROFL_PIPELINE_INFO("[trie] Trie structure:\n");

	//Start the recursion
	of1x_dump_leaf_trie(trie->root, 0, raw_nbo);

	//Now print the complete
	ROFL_PIPELINE_INFO("[trie] \n");
	ROFL_PIPELINE_INFO("[trie] Entries:\n");
	it = trie->entry;
	prev = NULL;
	next = trie->root;

	while(1){
		if(!it)
			it = of1x_find_reen_trie(&matches, &prev, &next,
								true,
								false,
								false);
		if(!it)
			break;

		//Dump
		ROFL_PIPELINE_INFO("[trie] ");
		of1x_dump_flow_entry(it, raw_nbo);

		it = it->next;
	}

}


//Define the matching algorithm struct
OF1X_REGISTER_MATCHING_ALGORITHM(trie) = {
	//Init and destroy hooks
	.init_hook = of1x_init_trie,
	.destroy_hook = of1x_destroy_trie,

	//Flow mods
	.add_flow_entry_hook = of1x_add_flow_entry_trie,
	.modify_flow_entry_hook = of1x_modify_flow_entry_trie,
	.remove_flow_entry_hook = of1x_remove_flow_entry_trie,

	//Stats
	.get_flow_stats_hook = of1x_get_flow_stats_trie,
	.get_flow_aggregate_stats_hook = of1x_get_flow_aggregate_stats_trie,

	//Find group related entries
	.find_entry_using_group_hook = of1x_find_entry_using_group_trie,

	//Dumping
	.dump_hook = of1x_dump_trie,
	.description = TRIE_DESCRIPTION,
};
