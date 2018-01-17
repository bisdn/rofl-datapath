#include "l2hash.h"

static of1x_switch_t* sw=NULL;

int set_up(){

	physical_switch_init();

	enum of1x_matching_algorithm_available ma_list[4]={of1x_l2hash_matching_algorithm, of1x_l2hash_matching_algorithm,
	of1x_l2hash_matching_algorithm, of1x_l2hash_matching_algorithm};

	//Create instance
	sw = of1x_init_switch("Test switch", OF_VERSION_12, SW_FLAVOR_GENERIC, 0x0101,4,ma_list);

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

void test_install_invalid_flowmods(){

	//Empty

	//Create a simple flow_mod
	of1x_flow_entry_t* entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//Empty
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) != ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//Port
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_port_in_match(1)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) != ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//ETH_SRC no wildcards
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_src_match(rand()%0xFFFFFFFF, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) != ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//ETH_DST with wildcards
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(rand()%0xFFFFFFFF, 0xFFFFFFFF0000)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) != ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//L3
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip4_dst_match(rand()%0xFFFFFFFF, 0xFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) != ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//ETH_DST + VLAN 1
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(0x12345678, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_vlan_vid_match(1620, 0xf00f, true)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) != ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);
}

void test_install_overlapping_specific(){

	unsigned int i, num_of_flows=rand()%20;
	of1x_flow_entry_t* entry;

	//Install N flowmods which identical => should put only one
	for(i=0;i<num_of_flows;i++){
		entry = of1x_init_flow_entry(false, /*builtin=*/false);
		CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(0x12345678, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);

		CU_ASSERT(entry != NULL);
		CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	}

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);

	//Uninstall all
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(0x12345678, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	rofl_of1x_fm_result_t specific_remove_result = of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY);
	CU_ASSERT( specific_remove_result == ROFL_OF1X_FM_SUCCESS ); //First must succeeed

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(0x12345678, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	specific_remove_result = of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY);
	CU_ASSERT( specific_remove_result == ROFL_OF1X_FM_SUCCESS ); //Second too according to spec (no entries)


	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);
}

void test_multiple_masks(){

	//Create a simple flow_mod
	of1x_flow_entry_t* entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);

	//ETH_DST with wildcards
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(0x12345678, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);

	//ETH_DST + VLAN 1
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_eth_dst_match(0x12345678, 0xFFFFFFFFFFFF)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_vlan_vid_match(1620, 0xffff, true)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 2);
}
