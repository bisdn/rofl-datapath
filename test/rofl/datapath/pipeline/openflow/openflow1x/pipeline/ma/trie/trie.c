#include "trie.h"
#include "rofl/datapath/pipeline/openflow/openflow1x/pipeline/matching_algorithms/trie/of1x_trie_ma.h"


static of1x_switch_t* sw = NULL;
static of1x_trie_t* trie = NULL;
static of1x_flow_table_t* table = NULL;

int set_up(){

	physical_switch_init();

	enum of1x_matching_algorithm_available ma_list[4]={of1x_trie_matching_algorithm, of1x_trie_matching_algorithm,
	of1x_trie_matching_algorithm, of1x_trie_matching_algorithm};

	//Create instance
	sw = of1x_init_switch("Test switch", OF_VERSION_12, SW_FLAVOR_GENERIC, 0x0101,4,ma_list);
	table = &sw->pipeline.tables[0];
	trie = (of1x_trie_t*)table->matching_aux[0];

	if(!sw)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int tear_down(){
	//Destroy the switch
	if(__of1x_destroy_switch(sw) != ROFL_SUCCESS)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static void clean_all(){
	of1x_flow_entry_t *entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, false, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
}

void test_install_empty_flowmods(){

	of1x_flow_entry_t *entry, *tmp;

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	entry->priority=100;
	//Force entry to have invalid values in next and prev
	entry->next = entry->prev = (void*)0x1;

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);

	CU_ASSERT(table->num_of_entries == 1);
	CU_ASSERT(trie->entry != NULL);
	if(trie->entry){
		CU_ASSERT(trie->entry->next == NULL);
		CU_ASSERT(trie->entry->prev == NULL);
		CU_ASSERT(trie->entry->priority == 100);
	}

	//Add a second entry with lower priority
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	entry->priority=100;
	entry->next = entry->prev = (void*)0x1;

	//Overlap must fail
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);
	entry->priority=99;

	//Add without overlap
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(table->num_of_entries == 2);
	CU_ASSERT(trie->entry != NULL);
	if(trie->entry){
		CU_ASSERT(trie->entry->prev == NULL);
		CU_ASSERT(trie->entry->priority == 100);
		CU_ASSERT(trie->entry->next != NULL);
		CU_ASSERT(trie->entry->next->prev == trie->entry);
		CU_ASSERT(trie->entry->next->next == NULL);
		CU_ASSERT(trie->entry->next->priority == 99);
	}

	//Add one with higher priority
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	entry->priority=110;
	entry->next = entry->prev = (void*)0x1;

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(table->num_of_entries == 3);
	CU_ASSERT(trie->entry != NULL);
	if(trie->entry){
		CU_ASSERT(trie->entry->prev == NULL);
		CU_ASSERT(trie->entry->priority == 110);
		CU_ASSERT(trie->entry->next != NULL);
		CU_ASSERT(trie->entry->next->priority == 100);
		CU_ASSERT(trie->entry->next->prev == trie->entry);
		CU_ASSERT(trie->entry->next->next != NULL);
		CU_ASSERT(trie->entry->next->next->priority == 99);
		CU_ASSERT(trie->entry->next->next->prev == trie->entry->next);
		CU_ASSERT(trie->entry->next->next->next == NULL);
	}

	//Add one in between
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	entry->priority=100;
	entry->next = entry->prev = (void*)0x1;

	//Overlap must fail
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);
	entry->priority=107;

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(table->num_of_entries == 4);
	CU_ASSERT(trie->entry != NULL);
	if(trie->entry){
		CU_ASSERT(trie->entry->prev == NULL);
		CU_ASSERT(trie->entry->priority == 110);
		CU_ASSERT(trie->entry->next != NULL);
		CU_ASSERT(trie->entry->next->priority == 107);
		CU_ASSERT(trie->entry->next->prev == trie->entry);
		CU_ASSERT(trie->entry->next->next != NULL);
		CU_ASSERT(trie->entry->next->next->priority == 100);
		CU_ASSERT(trie->entry->next->next->prev == trie->entry->next);
		CU_ASSERT(trie->entry->next->next->next != NULL);
		CU_ASSERT(trie->entry->next->next->next->prev == trie->entry->next->next);
		CU_ASSERT(trie->entry->next->next->next->priority == 99);
		CU_ASSERT(trie->entry->next->next->next->next == NULL);
	}

	//Override priority == 99
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	tmp = entry; //hold pointer
	CU_ASSERT(entry != NULL);
	entry->priority=99;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(table->num_of_entries == 4);
	CU_ASSERT(trie->entry != NULL);
	if(trie->entry){
		CU_ASSERT(trie->entry->next->next->next->priority == 99);
		CU_ASSERT(trie->entry->next->next->next == tmp);
	}

}

void test_install_flowmods(){

	of1x_flow_entry_t *entry, *tmp;

	/* ----- 1 ------ */

	//Create a flowmod with a single match
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	entry->priority=999;
	CU_ASSERT(entry != NULL);

	//192.168.0.1
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);

	tmp = entry;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(table->num_of_entries == 5);
	CU_ASSERT(trie->root->imp == 999);


	of1x_full_dump_switch(sw, false);

	//
	// Expected tree structure:
	//
	// - m:192.168.0.1/32 entry:1 imp:999


	CU_ASSERT(trie->root != NULL);
	CU_ASSERT(trie->root->parent == NULL);
	CU_ASSERT(trie->root->inner == NULL);
	CU_ASSERT(trie->root->entry == tmp);
	CU_ASSERT(trie->root->entry->next == NULL);
	CU_ASSERT(trie->root->next == NULL);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->imp == 999);

	/* ----- 2 ------ */

	//Add a flowmod that shares most of it (except two bits)
	//Creates an intermediate leaf
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//192.168.0.2
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80002, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=1999;

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(table->num_of_entries == 6);
	CU_ASSERT(trie->root->imp == 1999);

	of1x_full_dump_switch(sw, false);

	//
	// Expected tree structure:
	//
	// -m:192.168.0.0/30 entry:NULL imp:1999
	// -- m:192.168.0.2/32 entry:2 imp:1999
	// -- m:192.168.0.1/32 entry:1 imp:999

	CU_ASSERT(trie->root != NULL);
	CU_ASSERT(trie->root->parent == NULL);
	CU_ASSERT(trie->root->inner != NULL);
	CU_ASSERT(trie->root->entry == NULL);
	CU_ASSERT(trie->root->next == NULL);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFFFC));
	CU_ASSERT(trie->root->imp == 1999);
	CU_ASSERT(trie->root->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->imp == 999);
	CU_ASSERT(trie->root->inner->next->imp == 1999);
	CU_ASSERT(trie->root->inner->next->entry != NULL);
	CU_ASSERT(trie->root->inner->next->next == NULL);

	/* ----- 3 ------ */

	//Try overlapping
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//192.168.0.0/24
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80002, 0xFFFFFF00)) == ROFL_SUCCESS);
	entry->priority=1999;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);


	//
	// Expected tree structure:
	//
	// -m:192.168.0.0/24 entry:3 imp: 1999
	// --m:192.168.0.0/30 entry:NULL imp:1999
	// --- m:192.168.0.2/32 entry:2 imp:1999
	// --- m:192.168.0.1/32 entry:1 imp:999

	entry->priority=99; //deliverately clashing with empty entry
	tmp = entry;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	CU_ASSERT(trie->root->imp == 1999);

	of1x_full_dump_switch(sw, false);

	//Check 3
	CU_ASSERT(table->num_of_entries == 7);
	CU_ASSERT(trie->root->imp == 1999);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFF00));
	CU_ASSERT(trie->root->entry == tmp);
	CU_ASSERT(trie->root->entry->priority == 99);
	CU_ASSERT(trie->root->next == NULL);

	//Check intermediate
	CU_ASSERT(trie->root->inner != NULL);
	CU_ASSERT(trie->root->inner->imp == 1999);
	CU_ASSERT(trie->root->inner->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFC));
	CU_ASSERT(trie->root->inner->entry == NULL);

	//Check 1
	CU_ASSERT(trie->root->inner->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->inner == NULL);
	CU_ASSERT(trie->root->inner->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->entry->priority = 999);
	CU_ASSERT(trie->root->inner->inner->prev == NULL);
	CU_ASSERT(trie->root->inner->inner->parent == trie->root->inner);

	//Check 2
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->imp == 999);
	CU_ASSERT(trie->root->inner->inner->next->imp == 1999);
	CU_ASSERT(trie->root->inner->inner->next->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->entry->priority = 1999);
	CU_ASSERT(trie->root->inner->inner->next->prev == trie->root->inner->inner);
	CU_ASSERT(trie->root->inner->inner->next->next == NULL);
	CU_ASSERT(trie->root->inner->inner->parent == trie->root->inner);

	/* ----- 4 ------ */

	//Try overlapping
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//192.168.0.3
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80003, 0xFFFFFFFF)) == ROFL_SUCCESS);
	//192.168.0.1
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //explicitely use a clashing priority
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);
	entry->priority=3999;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);

	//
	// Expected tree structure:
	//
	/*
		Empty match entries:
		  * p: 110 (0x65fb10)
		  * p: 107 (0x6600f0)
		  * p: 100 (0x65ef50)
		  * p: 99 (0x6606d0)

		Match entries:
		 l:[IP4_SRC:0xc0a80000|0xffffff00],  imp: 3999 * p: 99 (0x661500)
		   l:[IP4_SRC:0xc0a80000|0xfffffffc],  imp: 3999 
		     l:[IP4_SRC:0xc0a80001|0xffffffff],  imp: 999 * p: 999 (0x65f530)
		     l:[IP4_SRC:0xc0a80002|0xfffffffe],  imp: 3999 
		       l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 1999 * p: 1999 (0x660da0)
		       l:[IP4_SRC:0xc0a80003|0xffffffff],  imp: 3999 
		         l:[IP4_DST:0xc0a80001|0xffffffff],  imp: 3999 * p: 3999 (0x661bd0)
 	*/

	CU_ASSERT(trie->root->imp == 3999);
	of1x_full_dump_switch(sw, false);

	//Check 3
	CU_ASSERT(table->num_of_entries == 8);
	CU_ASSERT(trie->root->imp == 3999);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFF00));
	CU_ASSERT(trie->root->entry == tmp);
	CU_ASSERT(trie->root->entry->priority == 99);
	CU_ASSERT(trie->root->next == NULL);

	//Check intermediate
	CU_ASSERT(trie->root->inner != NULL);
	CU_ASSERT(trie->root->inner->imp == 3999);
	CU_ASSERT(trie->root->inner->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFC));
	CU_ASSERT(trie->root->inner->entry == NULL);
	CU_ASSERT(trie->root->next == NULL);

	//Check 1
	CU_ASSERT(trie->root->inner->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->inner == NULL);

	//Check intermediate 2
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFE));
	CU_ASSERT(trie->root->inner->inner->next->imp == 3999);
	CU_ASSERT(trie->root->inner->inner->next->entry == NULL);
	CU_ASSERT(trie->root->next == NULL);

	//Check 2
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->inner == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->entry->priority == 1999);
	CU_ASSERT(trie->root->inner->inner->next->inner->imp == 1999);
	CU_ASSERT(trie->root->inner->inner->next->imp == 3999);

	//Check 4
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80003));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->entry == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->imp == 3999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry->priority = 3999);

	/* ----- 5 ------ */
	//Try overlapping
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//192.168.0.2/31
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80002, 0xFFFFFFFE)) == ROFL_SUCCESS);
	//192.168.0.10
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A8000A, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //explicitely use a clashing priority
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);
	entry->priority=4999;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(trie->root->imp == 4999);
	//
	// Expected tree structure:
	//
	/*
		Empty match entries:
		  * p: 110 (0x65fb10)
		  * p: 107 (0x6600f0)
		  * p: 100 (0x65ef50)
		  * p: 99 (0x6606d0)

		Match entries:
		 l:[IP4_SRC:0xc0a80000|0xffffff00],  imp: 4999 * p: 99 (0x661500)
		   l:[IP4_SRC:0xc0a80000|0xfffffffc],  imp: 4999 
		     l:[IP4_SRC:0xc0a80001|0xffffffff],  imp: 999 * p: 999 (0x65f530)
		     l:[IP4_SRC:0xc0a80002|0xfffffffe],  imp: 4999 
		       l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 1999 * p: 1999 (0x660da0)
		       l:[IP4_SRC:0xc0a80003|0xffffffff],  imp: 3999 
			 l:[IP4_DST:0xc0a80001|0xffffffff],  imp: 3999 * p: 3999 (0x661bd0)
		       l:[IP4_DST:0xc0a8000a|0xffffffff],  imp: 4999 * p: 4999 (0x662420)
 	*/
	//Check 3
	CU_ASSERT(table->num_of_entries == 9);
	CU_ASSERT(trie->root->imp == 4999);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFF00));
	CU_ASSERT(trie->root->entry == tmp);
	CU_ASSERT(trie->root->entry->priority == 99);
	CU_ASSERT(trie->root->next == NULL);

	//Check intermediate
	CU_ASSERT(trie->root->inner != NULL);
	CU_ASSERT(trie->root->inner->imp == 4999);
	CU_ASSERT(trie->root->inner->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFC));
	CU_ASSERT(trie->root->inner->entry == NULL);
	CU_ASSERT(trie->root->next == NULL);

	//Check 1
	CU_ASSERT(trie->root->inner->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->inner == NULL);

	//Check intermediate 2
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFE));
	CU_ASSERT(trie->root->inner->inner->next->imp == 4999);
	CU_ASSERT(trie->root->inner->inner->next->entry == NULL);
	CU_ASSERT(trie->root->next == NULL);

	//Check 2
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->inner == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->imp == 1999);
	CU_ASSERT(trie->root->inner->inner->next->inner->entry->priority == 1999);

	//Check 4
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80003));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->entry == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->imp == 3999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry->priority = 3999);

	//Check 5
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->match.__tern.value.u32 == HTONB32(0xC0A8000A));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->entry->priority = 4999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->next == NULL);

	/* ----- 6 ------ */
	//Try overlapping
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//MAC_SRC
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	//192.168.0.2
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80002, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //explicitely use a clashing priority
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);
	entry->priority=5999;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(trie->root->imp == 4999);
	CU_ASSERT(trie->root->next->imp == 5999);

	//
	// Expected tree structure:
	//
	/*
		Empty match entries:
		  * p: 110 (0x65fb10)
		  * p: 107 (0x6600f0)
		  * p: 100 (0x65ef50)
		  * p: 99 (0x6606d0)

		Match entries:
		 l:[IP4_SRC:0xc0a80000|0xffffff00],  imp: 4999 * p: 99 (0x661500)
		   l:[IP4_SRC:0xc0a80000|0xfffffffc],  imp: 4999 
		     l:[IP4_SRC:0xc0a80001|0xffffffff],  imp: 999 * p: 999 (0x65f530)
		     l:[IP4_SRC:0xc0a80002|0xfffffffe],  imp: 4999 
		       l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 1999 * p: 1999 (0x660da0)
		       l:[IP4_SRC:0xc0a80003|0xffffffff],  imp: 3999 
			 l:[IP4_DST:0xc0a80001|0xffffffff],  imp: 3999 * p: 3999 (0x661bd0)
		       l:[IP4_DST:0xc0a8000a|0xffffffff],  imp: 4999 * p: 4999 (0x662420)
		 l:[ETH_SRC:0xaabbccddeeff|0xffffffffffff],  imp: 5999 
		   l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 5999 * p: 5999 (0x662b50)
 	*/

	CU_ASSERT(table->num_of_entries == 10);
	CU_ASSERT(trie->root->imp == 4999);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFF00));
	CU_ASSERT(trie->root->entry == tmp);
	CU_ASSERT(trie->root->entry->priority == 99);
	CU_ASSERT(trie->root->next != NULL);

	//Check intermediate
	CU_ASSERT(trie->root->inner != NULL);
	CU_ASSERT(trie->root->inner->imp == 4999);
	CU_ASSERT(trie->root->inner->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFC));
	CU_ASSERT(trie->root->inner->entry == NULL);

	//Check 1
	CU_ASSERT(trie->root->inner->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->inner == NULL);

	//Check intermediate 2
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFE));
	CU_ASSERT(trie->root->inner->inner->next->imp == 4999);
	CU_ASSERT(trie->root->inner->inner->next->entry == NULL);

	//Check 2
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->inner == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->entry->priority == 1999);

	//Check 4
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80003));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->entry == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->imp == 3999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry->priority = 3999);

	//Check 5
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->match.__tern.value.u32 == HTONB32(0xC0A8000A));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->entry->priority = 4999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->next == NULL);

	//Check 6
	CU_ASSERT(trie->root->next != NULL);
	CU_ASSERT(trie->root->next->imp == 5999);
	CU_ASSERT(trie->root->next->match.__tern.value.u64 == OF1X_MAC_VALUE(HTONB64(0xAABBCCDDEEFF)));
	CU_ASSERT(trie->root->next->match.__tern.mask.u64 == OF1X_MAC_VALUE(HTONB64(0xFFFFFFFFFFFF)));
	CU_ASSERT(trie->root->next->entry == NULL);
	CU_ASSERT(trie->root->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->next->inner->entry != NULL);
	CU_ASSERT(trie->root->next->inner->entry->priority == 5999);

	/* ----- 7 ------ */
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //explicitely use a clashing priority
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, true,false) == ROFL_OF1X_FM_OVERLAP);
	entry->priority=6999;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(trie->root->imp == 4999);
	CU_ASSERT(trie->root->inner->imp == 4999);
	CU_ASSERT(trie->root->inner->inner->imp == 999);
	CU_ASSERT(trie->root->inner->inner->next->imp == 4999);
	CU_ASSERT(trie->root->next->imp == 6999);

	//
	// Expected tree structure:
	//
	/*
		Empty match entries:
		  * p: 110 (0x65fb10)
		  * p: 107 (0x6600f0)
		  * p: 100 (0x65ef50)
		  * p: 99 (0x6606d0)

		Match entries:
		 l:[IP4_SRC:0xc0a80000|0xffffff00],  imp: 4999 * p: 99 (0x661500)
		   l:[IP4_SRC:0xc0a80000|0xfffffffc],  imp: 4999 
		     l:[IP4_SRC:0xc0a80001|0xffffffff],  imp: 999 * p: 999 (0x65f530)
		     l:[IP4_SRC:0xc0a80002|0xfffffffe],  imp: 4999 
		       l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 1999 * p: 1999 (0x660da0)
		       l:[IP4_SRC:0xc0a80003|0xffffffff],  imp: 3999 
			 l:[IP4_DST:0xc0a80001|0xffffffff],  imp: 3999 * p: 3999 (0x661bd0)
		       l:[IP4_DST:0xc0a8000a|0xffffffff],  imp: 4999 * p: 4999 (0x662420)
		 l:[ETH_SRC:0xaab000000000|0xfff000000000],  imp: 6999 
		   l:[ETH_SRC:0xaabbccddeeff|0xffffffffffff],  imp: 5999 
		     l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 5999 * p: 5999 (0x662b50)
		   l:[ETH_SRC:0xaab0ccddeeff|0xfff0ffffffff],  imp: 6999 * p: 6999 (0x663310)
	*/

	CU_ASSERT(table->num_of_entries == 11);
	CU_ASSERT(trie->root->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->match.__tern.mask.u32 == HTONB32(0xFFFFFF00));
	CU_ASSERT(trie->root->entry == tmp);
	CU_ASSERT(trie->root->entry->priority == 99);
	CU_ASSERT(trie->root->next != NULL);

	//Check intermediate
	CU_ASSERT(trie->root->inner != NULL);
	CU_ASSERT(trie->root->inner->imp == 4999);
	CU_ASSERT(trie->root->inner->match.__tern.value.u32 == HTONB32(0xC0A80000));
	CU_ASSERT(trie->root->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFC));
	CU_ASSERT(trie->root->inner->entry == NULL);

	//Check 1
	CU_ASSERT(trie->root->inner->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->inner == NULL);

	//Check intermediate 2
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFE));
	CU_ASSERT(trie->root->inner->inner->next->entry == NULL);

	//Check 2
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->inner->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->inner == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->entry->priority == 1999);
	CU_ASSERT(trie->root->inner->inner->next->imp == 4999);

	//Check 4
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.value.u32 == HTONB32(0xC0A80003));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->entry == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->imp == 3999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.value.u32 == HTONB32(0xC0A80001));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->inner->entry->priority = 3999);

	//Check 5
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->match.__tern.value.u32 == HTONB32(0xC0A8000A));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->entry != NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->entry->priority = 4999);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->next->next == NULL);

	//Check intermediate 3
	CU_ASSERT(trie->root->next != NULL);
	CU_ASSERT(trie->root->next->imp == 6999);
	CU_ASSERT(trie->root->next->match.__tern.value.u64 == OF1X_MAC_VALUE(HTONB64(0xAAB000000000)));
	CU_ASSERT(trie->root->next->match.__tern.mask.u64 == OF1X_MAC_VALUE(HTONB64(0xFFF000000000)));
	CU_ASSERT(trie->root->next->entry == NULL);

	//Check 6
	CU_ASSERT(trie->root->next->inner->match.__tern.value.u64 == OF1X_MAC_VALUE(HTONB64(0xAABBCCDDEEFF)));
	CU_ASSERT(trie->root->next->inner->match.__tern.mask.u64 == OF1X_MAC_VALUE(HTONB64(0xFFFFFFFFFFFF)));
	CU_ASSERT(trie->root->next->inner->entry == NULL);
	CU_ASSERT(trie->root->next->inner->inner->match.__tern.value.u32 == HTONB32(0xC0A80002));
	CU_ASSERT(trie->root->next->inner->inner->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));
	CU_ASSERT(trie->root->next->inner->inner->entry != NULL);
	CU_ASSERT(trie->root->next->inner->inner->entry->priority == 5999);


	//Check 7
	CU_ASSERT(trie->root->next->inner->next->match.__tern.value.u64 == OF1X_MAC_VALUE(HTONB64(0xAAB0CCDDEEFF)));
	CU_ASSERT(trie->root->next->inner->next->match.__tern.mask.u64 == OF1X_MAC_VALUE(HTONB64(0xFFF0FFFFFFFF)));
	CU_ASSERT(trie->root->next->inner->next->entry != NULL);
	CU_ASSERT(trie->root->next->inner->next->entry->priority == 6999);

}

void test_remove_flowmods(){

	of1x_flow_entry_t *entry;

	//
	// Expected tree structure:
	//
	/*
		Empty match entries:
		  * p: 110 (0x65fb10)
		  * p: 107 (0x6600f0)
		  * p: 100 (0x65ef50)
		  * p: 99 (0x6606d0)

		Match entries:
		 l:[IP4_SRC:0xc0a80000|0xffffff00],  imp: 4999 * p: 99 (0x661500)
		   l:[IP4_SRC:0xc0a80000|0xfffffffc],  imp: 4999 
		     l:[IP4_SRC:0xc0a80001|0xffffffff],  imp: 999 * p: 999 (0x65f530)
		     l:[IP4_SRC:0xc0a80002|0xfffffffe],  imp: 4999 
		       l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 1999 * p: 1999 (0x660da0)
		       l:[IP4_SRC:0xc0a80003|0xffffffff],  imp: 3999 
			 l:[IP4_DST:0xc0a80001|0xffffffff],  imp: 3999 * p: 3999 (0x661bd0)
		       l:[IP4_DST:0xc0a8000a|0xffffffff],  imp: 4999 * p: 4999 (0x662420)
		 l:[ETH_SRC:0xaab000000000|0xfff000000000],  imp: 6999 
		   l:[ETH_SRC:0xaabbccddeeff|0xffffffffffff],  imp: 5999 
		     l:[IP4_SRC:0xc0a80002|0xffffffff],  imp: 5999 * p: 5999 (0x662b50)
		   l:[ETH_SRC:0xaab0ccddeeff|0xfff0ffffffff],  imp: 6999 * p: 6999 (0x663310)
	*/
	//clean the table
	clean_all();

	CU_ASSERT(trie->root == NULL);
	CU_ASSERT(table->num_of_entries == 0);

	of1x_full_dump_switch(sw, false);

	//Regenerate
	test_install_empty_flowmods();
	test_install_flowmods();

	of1x_full_dump_switch(sw, false);

	///
	/// remove only one of the flowmods without matches
	//
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//First try with an invalid priority
	entry->priority = 1;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 11);

	of1x_full_dump_switch(sw, false);
	entry->priority = 107;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 10);

	of1x_full_dump_switch(sw, false);

	entry->priority = 99;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 9);

	of1x_full_dump_switch(sw, false);
	entry->priority = 110;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 8);

	of1x_full_dump_switch(sw, false);
	entry->priority = 100;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 7);

	of1x_full_dump_switch(sw, false);

	///
	/// Remove now non specific, but with limited matches
	///
	entry->priority = 6789; //whatever
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80003, 0xFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 7);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, false, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 5);

	CU_ASSERT(trie->root->inner->inner->next->inner->next->next == NULL);
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.value.u32 == HTONB32(0xC0A8000A));
	CU_ASSERT(trie->root->inner->inner->next->inner->next->match.__tern.mask.u32 == HTONB32(0xFFFFFFFF));

	of1x_full_dump_switch(sw, false);


	///
	/// Remove specific; first clean
	///
	clean_all();

	CU_ASSERT(trie->root == NULL);
	CU_ASSERT(table->num_of_entries == 0);

	//Reset
	test_install_empty_flowmods();
	test_install_flowmods();

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	of1x_full_dump_switch(sw, false);

	entry->priority = 3999;
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80003, 0xFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 11);
	of1x_full_dump_switch(sw, false);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);

	//Remove with an invalid priority
	entry->priority = 1999;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 11);

	entry->priority = 3999;
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 10);

	of1x_full_dump_switch(sw, false);

	///
	/// Remove with out group
	///
	clean_all();

	CU_ASSERT(trie->root == NULL);
	CU_ASSERT(table->num_of_entries == 0);

	//Reset
	test_install_empty_flowmods();
	test_install_flowmods();

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	of1x_full_dump_switch(sw, false);

	entry->priority = 3999;
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_src_match(0xC0A80003, 0xFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 11);
	of1x_full_dump_switch(sw, false);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, true, 0x1, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(table->num_of_entries == 11);

	of1x_full_dump_switch(sw, false);

}

void test_regressions(){

	/*--- Add 2 exact flows with different priorities ---*/

	of1x_flow_entry_t* entry;

	//First entry
	clean_all();
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);

	//Repeat; should overwrite it
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //Same exact priority => replace
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);

	//Different priority; shall not overwrite it
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=199; //!
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 2);

	CU_ASSERT(trie->root->imp == 199);
	CU_ASSERT(trie->root->entry == NULL);
	CU_ASSERT(trie->root->inner->imp == 199);
	CU_ASSERT(trie->root->inner->entry != NULL);
	CU_ASSERT(trie->root->inner->entry->priority == 199);
	CU_ASSERT(trie->root->inner->entry->next != NULL);

	/*--- Add/Modify preserve statistics --*/

	//First entry
	clean_all();
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99;
	entry->stats.s.__internal[0].packet_count = 1;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);

	//Modify without reset_counters => pkt_count == 1
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //Same exact priority => replace
	entry->stats.s.__internal[0].packet_count = 0;
	CU_ASSERT(of1x_modify_flow_entry_table(&sw->pipeline, 0, &entry, false, false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);
	CU_ASSERT(trie->root->inner->entry->stats.s.__internal[0].packet_count == 1);

	//Add must clear stats
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(0xAABBCCDDEEFF, 0xFFF0FFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=99; //Same exact priority => replace
	entry->stats.s.__internal[0].packet_count = 0;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);
	CU_ASSERT(trie->root->inner->entry->stats.s.__internal[0].packet_count == 0);


}

void test_regression1(){

	of1x_flow_entry_t* entry;

	//First entry
	clean_all();
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80000, 0xFFFFFF00)) == ROFL_SUCCESS);
	entry->priority=32;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);

	//Add one with the same + 1 match
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80001, 0xFFFFFFFF)) == ROFL_SUCCESS);
	entry->priority=31; //Same exact priority => replace
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 2);

	//Remove intermediate, intermediate should be pruned
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(0xC0A80000, 0xFFFFFF00)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, false, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, false, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 0);
}

void test_regression2(){

	of1x_flow_entry_t* entry;

	//First entry
	clean_all();
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=0;
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == 1);

	//Add PORT_IN p=64
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=64;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(3));
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=64;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(4));
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	//Add PORT_IN p=128
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=128;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(1));
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=128;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(2));
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	//Add PORT_IN + ETH TYPE p = 32768
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=32768;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(1));
	of1x_add_match_to_entry(entry,of1x_init_eth_type_match(0x806));

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=32768;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(2));
	of1x_add_match_to_entry(entry,of1x_init_eth_type_match(0x806));

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);


	//Add PORT_IN + ETH TYPE + IP_PROTO + UDP_SRC + UDP_DST p = 32768
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	entry->priority=32768;
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(1));
	of1x_add_match_to_entry(entry,of1x_init_eth_type_match(0x800));
	of1x_add_match_to_entry(entry,of1x_init_ip_proto_match(17));

	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);


}

#define NUM_ENTRIES 260
void test_many_entries(){

	int i;
	of1x_flow_entry_t* entry;

	clean_all();

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//First fill in all the entries
	for(i=0;i<NUM_ENTRIES; i++){
		entry = of1x_init_flow_entry(false, /*builtin=*/false);
		of1x_add_match_to_entry(entry,of1x_init_port_in_match(i));
		CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	}
	of1x_full_dump_switch(sw, false);
	CU_ASSERT(table->num_of_entries == NUM_ENTRIES);

	fprintf(stderr, "Starting deletion...\n");

	clean_all();
	CU_ASSERT(table->num_of_entries == 0);
}

