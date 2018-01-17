#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include "CUnit/Basic.h"
#include "test_ipv6.h"
#include <rofl/datapath/pipeline/common/ternary_fields.h>

static of1x_switch_t* sw=NULL;

int ipv6_set_up(void){
	//this is the set up for all tests (its only called once before all test, not for each one)
	printf("ipv6 setting up\n");
	
	physical_switch_init();

	enum of1x_matching_algorithm_available ma_list[4]={of1x_loop_matching_algorithm, of1x_loop_matching_algorithm,
	of1x_loop_matching_algorithm, of1x_loop_matching_algorithm};

	//Create instance	
	sw = of1x_init_switch("Test switch", OF_VERSION_13, SW_FLAVOR_GENERIC, 0x0101, 4, ma_list);
	
	if(!sw)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}

int ipv6_tear_down(void){
	//this is the tear down for all tests (its only called once after all test, not for each one)
	printf("\nipv6 tear down\n");
	
	//Destroy the switch
	if(__of1x_destroy_switch(sw) != ROFL_SUCCESS)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}


void ipv6_basic_test(void){
	printf("ipv6 test passed!\n");
}

void ipv6_utern_test(void){
	bool res;
	utern_t tern, ex_tern, ex_tern_fail;
	uint128__t value = 				{{0x12,0x34,0x56,0x78,0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78,0xff,0xff,0x12,0x34}};
	uint128__t mask = 				{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00}};
	uint128__t ex_value = 			{{0x12,0x34,0x56,0x78,0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78,0xff,0xf0,0x12,0x34}};
	uint128__t ex_mask = 			{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00}};
	uint128__t ex_value_2 = 		{{0x12,0x34,0x56,0x7a,0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78,0xff,0xff,0x12,0x34}};
	uint128__t ex_value_3 = 		{{0x12,0x34,0x56,0x78,0xff,0xff,0xff,0xff,0x12,0x34,0x56,0x78,0xff,0xff,0x12,0x3f}};
	uint128__t ex_value_fail = 		{{0x12,0x34,0x56,0x78,0xff,0xff,0xff,0xfc,0x12,0x34,0x56,0x78,0xff,0xff,0x12,0x34}};
	uint128__t ex_mask_fail = 		{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00}};
	
	__init_utern128(&tern,value,mask);
	
	res = __utern_compare128(&tern,&value);
	CU_ASSERT(res==true);
	
	res = __utern_compare128(&tern,&ex_value);
	CU_ASSERT(res==false);
	
	res = __utern_compare128(&tern,&ex_value_2);
	CU_ASSERT(res==false);
	
	res = __utern_compare128(&tern,&ex_value_3);
	CU_ASSERT(res==true);
	
	__init_utern128(&ex_tern,ex_value,ex_mask);
	res = __utern_is_contained(&ex_tern,&tern);
	CU_ASSERT(res==true);
	
	__init_utern128(&ex_tern_fail,ex_value_fail,ex_mask_fail);
	res = __utern_is_contained(&ex_tern_fail,&tern);
	CU_ASSERT(res==false);
	
}


/*TODO do tests using masks that doesn't make sense (not continuous ones) to check for failure*/

void ipv6_install_flow_mod(void){
	printf("ipv6 test flow mod\n");
	uint128__t value; UINT128__T_HI(value) = 0xffffffffffffffff; UINT128__T_LO(value) = 0xffffffffffff1234;
	uint128__t mask;  UINT128__T_HI(mask) = 0xffffffffffffffff; UINT128__T_LO(mask) = 0xffffffffffff0000;

	//Create a simple flow_mod
	of1x_flow_entry_t* entry = of1x_init_flow_entry(false, /*builtin=*/false);
	CU_ASSERT(entry != NULL);	
	entry->cookie_mask = 0x1;
	
	//add IPv6 match
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_src_match(value,mask)) == ROFL_SUCCESS);
	
	//Install
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	
	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);
	
	//New entry	
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	entry->cookie_mask = 0x1;
	//add IPv6 match
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_src_match(value,mask)) == ROFL_SUCCESS);
	
	//Uninstall (specific)	
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	
	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);
	
	
}

void ipv6_install_flow_mod_complete(void){
	printf("ipv6 test flow mod\n");
	uint128__t value128; UINT128__T_HI(value128) = 0xffffffffffffffff; UINT128__T_LO(value128) = 0xffffffffffff1234;
	uint128__t mask128; UINT128__T_HI(mask128) = 0xffffffffffffffff; UINT128__T_LO(mask128) = 0xffffffffffff0000;
	uint64_t value64 = 0xffffffffffff1234, mask64 = 0xffffffffffff0000;
	
	//Create a simple flow_mod
	of1x_flow_entry_t* entry = of1x_init_flow_entry(false, /*builtin=*/false);
	
	CU_ASSERT(entry != NULL);	
	
	entry->cookie_mask = 0x1;
	
	//add IPv6 match
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_src_match(value128,mask128)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_dst_match(value128,mask128)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_flabel_match(value64, mask64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_nd_target_match(value128)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_nd_sll_match(value64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_nd_tll_match(value64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_exthdr_match(value64,mask64)) == ROFL_SUCCESS);
	
	//Install
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	
	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);
	
	//New entry	
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	entry->cookie_mask = 0x1;
	//add IPv6 match
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_src_match(value128,mask128)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_dst_match(value128,mask128)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_flabel_match(value64,mask64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_nd_target_match(value128)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_nd_sll_match(value64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_nd_tll_match(value64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_ip6_exthdr_match(value64,mask64)) == ROFL_SUCCESS);
	

	//Uninstall (specific)	
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	
	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);
	
	
}

void icmpv6_install_flow_mod_complete(void){
	printf("ipv6 test flow mod\n");
	uint64_t value64 = 0xffffffffffff1234;
	
	//Create a simple flow_mod
	of1x_flow_entry_t* entry = of1x_init_flow_entry(false, /*builtin=*/false);
	
	CU_ASSERT(entry != NULL);	
	
	entry->cookie_mask = 0x1;
	
	//add IPv6 match
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_icmpv6_type_match(value64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_icmpv6_code_match(value64)) == ROFL_SUCCESS);
	
	//Install
	CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	CU_ASSERT(entry == NULL);
	
	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);
	
	//New entry	
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	
	//add IPv6 match
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_icmpv6_type_match(value64)) == ROFL_SUCCESS);
	CU_ASSERT(of1x_add_match_to_entry(entry,of1x_init_icmpv6_code_match(value64)) == ROFL_SUCCESS);
	

	//Uninstall (specific)	
	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, entry, STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);
	
	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);
}

void ipv6_process_packet(void){
	/*Create test to process a packet with ipv6 matches*/
	
	
}
