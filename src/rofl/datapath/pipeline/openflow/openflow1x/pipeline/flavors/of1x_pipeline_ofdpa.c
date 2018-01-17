#include "of1x_pipeline_ofdpa.h"
#include "../of1x_instruction.h"
#include "../of1x_timers.h"
#include "../of1x_action.h"
#include "../../../../platform/lock.h"
#include "../../../../platform/likely.h"
#include "../../../../platform/memory.h"
#include "../../../../platform/atomic_operations.h"
#include "../../../../common/protocol_constants.h"
#include "../../of1x_async_events_hooks.h"
#include "../matching_algorithms/matching_algorithms.h"
#include "../../of1x_switch.h"

#include "../../../../util/logging.h"

static const uint16_t ETH_TYPE_VLAN_CTAG_8100 = 0x8100;

ofdpa_table_setting_t ofdpa_table_settings[] = {
	{ OFDPA_INGRESS_PORT_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_VLAN_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_VLAN1_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_MPLS_L2_PORT_FLOW_TABLE, of1x_loop_matching_algorithm}, //TODO
	{ OFDPA_TERMINATION_MAC_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_MPLS0_FLOW_TABLE, of1x_loop_matching_algorithm}, //TODO
	{ OFDPA_MPLS1_FLOW_TABLE, of1x_loop_matching_algorithm}, //TODO
	{ OFDPA_MPLS2_FLOW_TABLE, of1x_loop_matching_algorithm}, //TODO
	{ OFDPA_MAINTENANCE_POINT_FLOW_TABLE, of1x_loop_matching_algorithm}, //TODO
	{ OFDPA_UNICAST_ROUTING_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_MULTICAST_ROUTING_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_BRIDGING_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_POLICY_ACL_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_COLOR_BASED_ACTIONS_FLOW_TABLE, of1x_loop_matching_algorithm}, //TODO
	{ OFDPA_EGRESS_VLAN_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_EGRESS_VLAN1_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_EGRESS_MAINTENANCE_POINT_FLOW_TABLE, of1x_loop_matching_algorithm},
	{ OFDPA_SOURCE_MAC_LEARNING_FLOW_TABLE, of1x_loop_matching_algorithm},
};

rofl_result_t __of1x_init_pipeline_ofdpa(struct of1x_pipeline* pipeline, const unsigned int num_of_tables_ignored, enum of1x_matching_algorithm_available* list_ignored){

	int table_index;

	unsigned int num_of_tables = sizeof(ofdpa_table_settings) / sizeof(ofdpa_table_setting_t);

	//Verify params
	if( ! (num_of_tables <= OF1X_MAX_FLOWTABLES && num_of_tables > 0) )
		return ROFL_FAILURE;

	//Fill in
	pipeline->num_of_tables = num_of_tables;
	pipeline->num_of_buffers = 0; //Should be filled in the post_init hook
	pipeline->first_table_index = OF1X_FIRST_FLOW_TABLE_INDEX;

	pipeline->ops.pre_flow_add_hook     = &__of1x_pipeline_ofdpa_flow_add;
	pipeline->ops.pre_flow_modify_hook  = &__of1x_pipeline_ofdpa_flow_modify;
	pipeline->ops.pre_flow_delete_hook  = &__of1x_pipeline_ofdpa_flow_remove;
	pipeline->ops.pre_group_add_hook    = &__of1x_pipeline_ofdpa_group_add;
	pipeline->ops.pre_group_modify_hook = &__of1x_pipeline_ofdpa_group_modify;
	pipeline->ops.pre_group_delete_hook = &__of1x_pipeline_ofdpa_group_delete;

	//Allocate tables and initialize
	pipeline->tables = (of1x_flow_table_t*)platform_malloc_shared(sizeof(of1x_flow_table_t)*num_of_tables);

	if(!pipeline->tables){
		return ROFL_FAILURE;
	}

	/*
	* Setting default capabilities and miss_send_lent. driver can afterwards
	* modify them at its will, via the hook.
	*/

	//Set datapath capabilities
	pipeline->capabilities = 	OF1X_CAP_FLOW_STATS |
					OF1X_CAP_TABLE_STATS |
					OF1X_CAP_PORT_STATS |
					OF1X_CAP_GROUP_STATS |
					//OF1X_CAP_IP_REASM |
					OF1X_CAP_QUEUE_STATS;
					//OF1X_CAP_ARP_MATCH_IP;

	//TODO: Evaluate if OF1X_CAP_PORT_BLOCKED should be added by default

	//Set MISS-SEND length to default
	pipeline->miss_send_len = OF1X_DEFAULT_MISS_SEND_LEN;

	//init groups
	pipeline->groups = of1x_init_group_table(pipeline);


	for(table_index=0;table_index<num_of_tables;table_index++){

		unsigned int table_index_next = 0;
		unsigned int table_number = ofdpa_table_settings[table_index].table_number; //OF table number
		enum of1x_matching_algorithm_available ma_alg = ofdpa_table_settings[table_index].ma_alg;

		//dummy
		bitmap256_t goto_tables;
		bitmap256_clean(&goto_tables);

		if( (ma_alg >= of1x_matching_algorithm_count) ||
			(__of1x_init_table(pipeline, &pipeline->tables[table_index], table_number, table_index, table_index_next, goto_tables, ma_alg) != ROFL_SUCCESS)
		){
			ROFL_PIPELINE_ERR("Creation of table #%d has failed in logical switch %s. This might be due to an invalid Matching Algorithm or that the system has run out of memory. Aborting Logical Switch creation\n",table_index,pipeline->sw->name);
			//Destroy already allocated tables
			for(--table_index; table_index>=0; table_index--){
				__of1x_destroy_table(&pipeline->tables[table_index]);
			}

			platform_free_shared(pipeline->tables);
			return ROFL_FAILURE;
		}
	}

	__of1x_set_pipeline_ofdpa_tables_defaults(pipeline, pipeline->sw->of_ver);

	return ROFL_SUCCESS;
}

rofl_result_t __of1x_destroy_pipeline_ofdpa(of1x_pipeline_t* pipeline){

	int i;

	//destroy groups
	of1x_destroy_group_table(pipeline->groups);

	for(i=0; i<pipeline->num_of_tables; i++){
		//We don't care about errors here, maybe add trace TODO
		__of1x_destroy_table(&pipeline->tables[i]);
	}

	//Now release table resources (allocated as single block)
	platform_free_shared(pipeline->tables);

	return ROFL_SUCCESS;
}


//Purge of all entries in the pipeline (reset)
rofl_result_t __of1x_purge_pipeline_ofdpa_entries(of1x_pipeline_t* pipeline){

	unsigned int i;
	rofl_result_t result = ROFL_SUCCESS;
	of1x_flow_entry_t* flow_entry;
	of1x_group_table_t* group_entry;

	//Create empty entries
	flow_entry = of1x_init_flow_entry(false, false);
	group_entry = of1x_init_group_table(pipeline);

	if( unlikely(flow_entry==NULL) )
		return ROFL_FAILURE;
	if( unlikely(group_entry==NULL) ){
		of1x_destroy_flow_entry(flow_entry);
		return ROFL_FAILURE;
	}

	//Purge flow_mods
	for(i=OF1X_FIRST_FLOW_TABLE_INDEX; i < pipeline->num_of_tables ; i++){
		//Purge flow_mods
		if(of1x_remove_flow_entry_table(pipeline, i, flow_entry, NOT_STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY) != ROFL_OF1X_FM_SUCCESS){
			result = ROFL_FAILURE;
			break;
		}
	}

	//Purge group mods
	if(result == ROFL_SUCCESS){
		if(of1x_group_delete(pipeline, group_entry, OF1X_GROUP_ANY) != ROFL_OF1X_GM_SUCCESS)
			result = ROFL_FAILURE;
	}

	//Destroy entries
	of1x_destroy_flow_entry(flow_entry);
	of1x_destroy_group_table(group_entry);

	return result;
}

//Set the default tables(flow and group tables) configuration according to the new version
rofl_result_t __of1x_set_pipeline_ofdpa_tables_defaults(of1x_pipeline_t* pipeline, of_version_t version){

	unsigned int table_index;

	for(table_index=0;table_index<pipeline->num_of_tables;table_index++){

		//set ofdpa table defaults
		of1x_flow_table_t *table = &pipeline->tables[table_index];
		switch(version){
		case OF_VERSION_13:
			__ofdpa_set_table_defaults(table);
			break;
		default:
			return ROFL_FAILURE;
		}
	}

	//set ofdpa group table defaults
	switch(version){
	case OF_VERSION_13:
		__of13_set_group_table_defaults(pipeline->groups);
		bitmap128_set(&(pipeline->groups->config.supported_actions), OF1X_AT_SET_FIELD_OFDPA_VRF);
		bitmap128_set(&(pipeline->groups->config.supported_actions), OF1X_AT_SET_FIELD_OFDPA_OVID);
		bitmap128_set(&(pipeline->groups->config.supported_actions), OF1X_AT_SET_FIELD_OFDPA_ALLOW_VLAN_TRANSLATION);
		//we reenable OF1X_AT_GROUP here in order to support multi-chained groups, DO NOT DO THIS OUTSIDE OF OFDPA PIPELINE FLAVOUR!!!
		bitmap128_set(&(pipeline->groups->config.supported_actions), OF1X_AT_GROUP);
		break;
	default:
		return ROFL_FAILURE;
	}

	return ROFL_SUCCESS;
}


//Set OFDPA table defaults
rofl_result_t __ofdpa_set_table_defaults(of1x_flow_table_t* table){

	if (table == NULL){
		return ROFL_FAILURE;
	}

	switch (table->number){
	case OFDPA_INGRESS_PORT_FLOW_TABLE:
		return __ofdpa_set_ingress_port_table_defaults(table);
	case OFDPA_VLAN_FLOW_TABLE:
		return __ofdpa_set_vlan_table_defaults(table);
	case OFDPA_VLAN1_FLOW_TABLE:
		return __ofdpa_set_vlan1_table_defaults(table);
	case OFDPA_TERMINATION_MAC_FLOW_TABLE:
		return __ofdpa_set_termination_mac_table_defaults(table);
	case OFDPA_BRIDGING_FLOW_TABLE:
		return __ofdpa_set_bridging_table_defaults(table);
	case OFDPA_UNICAST_ROUTING_FLOW_TABLE:
		return __ofdpa_set_unicast_routing_table_defaults(table);
	case OFDPA_MULTICAST_ROUTING_FLOW_TABLE:
		return __ofdpa_set_multicast_routing_table_defaults(table);
	case OFDPA_POLICY_ACL_FLOW_TABLE:
		return __ofdpa_set_policy_acl_table_defaults(table);
	case OFDPA_EGRESS_VLAN_FLOW_TABLE:
		return __ofdpa_set_egress_vlan_table_defaults(table);
	case OFDPA_EGRESS_VLAN1_FLOW_TABLE:
		return __ofdpa_set_egress_vlan1_table_defaults(table);
	default:{
		bitmap256_t *goto_tables = &(table->config.goto_tables);
		bitmap256_clean(goto_tables);

		//Set default behaviour MISS drop
		table->default_action = OF1X_TABLE_MISS_DROP;

		//Set default table index to 0 (no continuation)
		table->table_index_next = 0;

		//Match
		bitmap128_clean(&table->config.match);

		//Wildcards
		bitmap128_clean(&table->config.wildcards);

		//Apply actions
		bitmap128_clean(&table->config.apply_actions);

		//Write actions
		bitmap128_clean(&table->config.write_actions);

		//METADATA (full metadata support)
		table->config.metadata_match = 0x0ULL;
		table->config.metadata_write = 0x0ULL;

		//Instructions
		table->config.instructions = 0;
	};
	}

	return ROFL_SUCCESS;
};

rofl_result_t __ofdpa_set_ingress_port_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);
	of1x_flow_entry_t *entry;
	of1x_match_t *match;

	bitmap256_clean(goto_tables);

	bitmap256_set(goto_tables, OFDPA_VLAN_FLOW_TABLE);
	bitmap256_set(goto_tables, OFDPA_BRIDGING_FLOW_TABLE);

	//no continuation in next table, INGRESS_PORT table uses a default flow entry (see below)
	table->table_index_next = 0;

	//Set default behaviour MISS drop
	table->default_action = OF1X_TABLE_MISS_DROP;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_IN_PORT);
	bitmap128_set(&table->config.match, OF1X_MATCH_TUNNEL_ID);
	//extension for PUSH_VLAN
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_TUNNEL_ID);
	//extension for PUSH_VLAN
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_VLAN_VID);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_VRF);
	//extension for PUSH_VLAN
	bitmap128_set(&table->config.apply_actions, OF1X_AT_PUSH_VLAN);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_VLAN_VID);

	//Write actions
	bitmap128_clean(&table->config.write_actions);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);


	//Fill in default flow entry for non datacenter ethernet frames
	{
		//Create flow entry
		if ((entry = of1x_init_flow_entry(/*notify_removal=*/false, /*builtin=*/true)) == NULL) {
			return ROFL_FAILURE;
		}

		entry->priority = 0;
		entry->cookie = 0ULL;
		entry->cookie_mask = 0ULL;
		entry->flags = 0;
		entry->timer_info.idle_timeout = 0;
		entry->timer_info.hard_timeout = 0;

		//Match TUNNEL_ID
		if ((match = of1x_init_tunnel_id_match(/*tunnel_id=*/0, /*mask=*/OF1X_8_BYTE_MASK)) == NULL) {
			return ROFL_FAILURE;
		}
		if (of1x_add_match_to_entry(entry, match) != ROFL_SUCCESS){
			return ROFL_FAILURE;
		}

		//Instruction GOTO_TABLE
		of1x_add_instruction_to_group(
				&(entry->inst_grp),
				OF1X_IT_GOTO_TABLE,
				NULL,
				NULL,
				NULL,
				/*go_to_table*/OFDPA_VLAN_FLOW_TABLE);

		if (of1x_add_flow_entry_table(table->pipeline, table->number, &entry, false, true) != ROFL_OF1X_FM_SUCCESS){
			return ROFL_FAILURE;
		}
	}

#if 0
	//Fill in default flow entry for TAGGED ethernet frames
	{
		const uint16_t OFPVID_PRESENT = 0x1000;

		//Create flow entry
		if ((entry = of1x_init_flow_entry(/*notify_removal=*/false, /*builtin=*/true)) == NULL) {
			return ROFL_FAILURE;
		}

		entry->priority = 0;
		entry->cookie = 0ULL;
		entry->cookie_mask = 0ULL;
		entry->flags = 0;
		entry->timer_info.idle_timeout = 0;
		entry->timer_info.hard_timeout = 0;

		//Match TUNNEL_ID
		if ((match = of1x_init_tunnel_id_match(/*tunnel_id=*/0, /*mask=*/OF1X_8_BYTE_MASK)) == NULL) {
			return ROFL_FAILURE;
		}
		if (of1x_add_match_to_entry(entry, match) != ROFL_SUCCESS){
			return ROFL_FAILURE;
		}

		//Match VLAN_VID=ANY
		if ((match = of1x_init_vlan_vid_match(OFPVID_PRESENT, /*mask=*/OFPVID_PRESENT, OF1X_MATCH_VLAN_ANY))==NULL){
			return ROFL_FAILURE;
		}
		if (of1x_add_match_to_entry(entry, match) != ROFL_SUCCESS){
			return ROFL_FAILURE;
		}

		//Instruction GOTO_TABLE
		of1x_add_instruction_to_group(
				&(entry->inst_grp),
				OF1X_IT_GOTO_TABLE,
				NULL,
				NULL,
				NULL,
				/*go_to_table*/OFDPA_VLAN_FLOW_TABLE);

		if (of1x_add_flow_entry_table(table->pipeline, table->number, &entry, false, true) != ROFL_OF1X_FM_SUCCESS){
			return ROFL_FAILURE;
		}
	}

	//Fill in default flow entry for UNTAGGED ethernet frames
	{
		of1x_packet_action_t *action;
		of1x_action_group_t *apply_actions;
		wrap_uint_t field;


		//Create flow entry
		if ((entry = of1x_init_flow_entry(/*notify_removal=*/false, /*builtin=*/true)) == NULL) {
			return ROFL_FAILURE;
		}

		entry->priority = 0;
		entry->cookie = 0ULL;
		entry->cookie_mask = 0ULL;
		entry->flags = 0;
		entry->timer_info.idle_timeout = 0;
		entry->timer_info.hard_timeout = 0;

		//Match TUNNEL_ID
		if ((match = of1x_init_tunnel_id_match(/*tunnel_id=*/0, /*mask=*/OF1X_8_BYTE_MASK)) == NULL) {
			return ROFL_FAILURE;
		}
		if (of1x_add_match_to_entry(entry, match) != ROFL_SUCCESS){
			return ROFL_FAILURE;
		}

		//Match VLAN_VID=NONE
		if ((match = of1x_init_vlan_vid_match(0, /*mask=*/OF1X_VLAN_ID_MASK, OF1X_MATCH_VLAN_NONE))==NULL){
			return ROFL_FAILURE;
		}
		if (of1x_add_match_to_entry(entry, match) != ROFL_SUCCESS){
			return ROFL_FAILURE;
		}

		//Create action group for APPLY_ACTIONS
		if((apply_actions=of1x_init_action_group(0))==NULL){
			return ROFL_FAILURE;
		}

		//Action PUSH_VLAN
		field.u16 = ETH_TYPE_VLAN_CTAG_8100;
		if((action=of1x_init_packet_action(OF1X_AT_PUSH_VLAN, field, 0x0))==NULL){
			return ROFL_FAILURE;
		}
		of1x_push_packet_action_to_group(apply_actions, action);

		//Action SET_VLAN_VID=0
		field.u16 = 0;
		if((action=of1x_init_packet_action(OF1X_AT_SET_FIELD_VLAN_VID, field, 0x0))==NULL){
			return ROFL_FAILURE;
		}
		of1x_push_packet_action_to_group(apply_actions, action);

		//Instruction GOTO_TABLE
		of1x_add_instruction_to_group(
				&(entry->inst_grp),
				OF1X_IT_GOTO_TABLE,
				NULL,
				NULL,
				NULL,
				/*go_to_table*/OFDPA_VLAN_FLOW_TABLE);

		//Instruction APPLY_ACTIONS
		of1x_add_instruction_to_group(
				&(entry->inst_grp),
				OF1X_IT_APPLY_ACTIONS,
				apply_actions,
				NULL,
				NULL,
				0);

		if (of1x_add_flow_entry_table(table->pipeline, table->number, &entry, false, true) != ROFL_OF1X_FM_SUCCESS){
			return ROFL_FAILURE;
		}
	}
#endif
	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_vlan_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);

	bitmap256_set(goto_tables, OFDPA_VLAN1_FLOW_TABLE);
	bitmap256_set(goto_tables, OFDPA_TERMINATION_MAC_FLOW_TABLE);

	//no continuation in next table
	table->table_index_next = 0;

	//Set default behaviour MISS drop
	table->default_action = OF1X_TABLE_MISS_DROP;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_IN_PORT);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_VLAN_VID);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_VRF);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_OVID);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_PUSH_VLAN);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_POP_VLAN);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_MPLS_L2_PORT);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_TUNNEL_ID);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_LMEP_ID);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	//bitmap128_set(&table->config.write_actions, OF1X_AT_OFDPA_OAM_LM_TX_Count);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_WRITE_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_vlan1_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);

	bitmap256_set(goto_tables, OFDPA_TERMINATION_MAC_FLOW_TABLE);
	//bitmap256_set(goto_tables, OFDPA_MAINTENANCE_POINT_FLOW_TABLE);
	//bitmap256_set(goto_tables, OFDPA_MPLS_L2_PORT_FLOW_TABLE);

	//no continuation in next table
	table->table_index_next = 0;

	//Set default behaviour MISS drop
	table->default_action = OF1X_TABLE_MISS_DROP;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_IN_PORT);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_OVID);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE); //only allowed value: 0x8902 (OAM), only valid for OAM frames
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST); //only valid for OAM frames

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_VLAN_VID);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_VRF);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_PUSH_VLAN);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_POP_VLAN);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_MPLS_L2_PORT);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_TUNNEL_ID);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_LMEP_ID);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	//bitmap128_set(&table->config.write_actions, OF1X_AT_OFDPA_OAM_LM_TX_Count);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_WRITE_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_termination_mac_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);
	bitmap256_set(goto_tables, OFDPA_BRIDGING_FLOW_TABLE);
	bitmap256_set(goto_tables, OFDPA_UNICAST_ROUTING_FLOW_TABLE);
	bitmap256_set(goto_tables, OFDPA_MULTICAST_ROUTING_FLOW_TABLE);

	//continue in OFDPA_BRIDGING_FLOW_TABLE (table_index=11)
	table->table_index_next = 11;

	//Set default behaviour MISS continue
	table->default_action = OF1X_TABLE_MISS_CONTINUE;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_IN_PORT);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_DST);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_OUTPUT); //only valid destination: CONTROLLER

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	//bitmap128_set(&table->config.write_actions, OF1X_AT_OFDPA_OAM_LM_TX_Count);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_bridging_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);
	bitmap256_set(goto_tables, OFDPA_POLICY_ACL_FLOW_TABLE);

	//continue in OFDPA_POLICY_ACL_FLOW_TABLE (table_index=12)
	table->table_index_next = 12;

	//Set default behaviour MISS continue
	table->default_action = OF1X_TABLE_MISS_CONTINUE;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_TUNNEL_ID);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_DST);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_TUNNEL_ID);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_OUTPUT); //only valid destination: CONTROLLER

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	bitmap128_set(&table->config.write_actions, OF1X_AT_OUTPUT);
	bitmap128_set(&table->config.write_actions, OF1X_AT_GROUP);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_WRITE_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_unicast_routing_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);
	bitmap256_set(goto_tables, OFDPA_POLICY_ACL_FLOW_TABLE);

	//continue in OFDPA_POLICY_ACL_FLOW_TABLE (table_index=12)
	table->table_index_next = 12;

	//Set default behaviour MISS continue
	table->default_action = OF1X_TABLE_MISS_CONTINUE;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_VRF);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_DST);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	bitmap128_set(&table->config.write_actions, OF1X_AT_OUTPUT);
	bitmap128_set(&table->config.write_actions, OF1X_AT_DEC_NW_TTL);
	//TODO: MTU check vendor extension

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_CLEAR_ACTIONS) |
					(1 << OF1X_IT_WRITE_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_multicast_routing_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);
	bitmap256_set(goto_tables, OFDPA_POLICY_ACL_FLOW_TABLE);

	//continue in OFDPA_POLICY_ACL_FLOW_TABLE (table_index=12)
	table->table_index_next = 12;

	//Set default behaviour MISS continue
	table->default_action = OF1X_TABLE_MISS_CONTINUE;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_VRF);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV4_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_DST);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV4_SRC);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_SRC);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	bitmap128_set(&table->config.write_actions, OF1X_AT_GROUP);
	bitmap128_set(&table->config.write_actions, OF1X_AT_DEC_NW_TTL);
	//TODO: MTU check vendor extension

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_WRITE_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_policy_acl_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);
	of1x_flow_entry_t *entry;

	bitmap256_clean(goto_tables);
	//bitmap256_set(goto_tables, OFDPA_COLOR_BASED_ACTIONS_FLOW_TABLE);
	bitmap256_set(goto_tables, OFDPA_EGRESS_VLAN_FLOW_TABLE);
	bitmap256_set(goto_tables, OFDPA_EGRESS_VLAN1_FLOW_TABLE);
	//bitmap256_set(goto_tables, OFDPA_EGRESS_MAINTENANCE_POINT_FLOW_TABLE);

	//no continuation in next table
	table->table_index_next = 0;

	//Set default behaviour MISS drop
	table->default_action = OF1X_TABLE_MISS_DROP;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_IN_PORT);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_PCP);
	//bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_DEI); //TODO
	bitmap128_set(&table->config.match, OF1X_MATCH_TUNNEL_ID);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_VRF);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV4_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_ARP_SPA);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_FLABEL);
	bitmap128_set(&table->config.match, OF1X_MATCH_IPV6_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_IP_PROTO);
	bitmap128_set(&table->config.match, OF1X_MATCH_IP_DSCP);
	bitmap128_set(&table->config.match, OF1X_MATCH_IP_ECN);
	bitmap128_set(&table->config.match, OF1X_MATCH_TCP_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_UDP_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_SCTP_SRC);
	bitmap128_set(&table->config.match, OF1X_MATCH_ICMPV4_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_ICMPV6_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_TCP_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_UDP_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_SCTP_DST);
	bitmap128_set(&table->config.match, OF1X_MATCH_ICMPV4_CODE);
	bitmap128_set(&table->config.match, OF1X_MATCH_ICMPV6_CODE);
	//bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_MPLS_L2_PORT); //TODO

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_SRC);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_DST);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV4_SRC);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ARP_SPA);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV4_DST);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_SRC);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_FLABEL);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_IPV6_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_COLOR); //TODO
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_COLOR_ACTIONS_INDEX); //TODO
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_TRAFFIC_CLASS); //TODO
	/* this is invalid according to the OFDPA2 specification, but is used by baseboxd, so we add it here for testing */
	bitmap128_set(&table->config.apply_actions, OF1X_AT_GROUP);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_OUTPUT);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_QUEUE);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	bitmap128_set(&table->config.write_actions, OF1X_AT_GROUP);
	bitmap128_set(&table->config.write_actions, OF1X_AT_OUTPUT);
	bitmap128_set(&table->config.write_actions, OF1X_AT_SET_QUEUE);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_CLEAR_ACTIONS) |
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_WRITE_ACTIONS) |
					//(1 << OF1X_IT_METER) |
					(1 << OF1X_IT_GOTO_TABLE);

	//Fill in default flow entry for TABLE-MISS
	{
		//Create flow entry
		if ((entry = of1x_init_flow_entry(/*notify_removal=*/false, /*builtin=*/true)) == NULL) {
			return ROFL_FAILURE;
		}

		entry->priority = 0;
		entry->cookie = 0ULL;
		entry->cookie_mask = 0ULL;
		entry->flags = 0;
		entry->timer_info.idle_timeout = 0;
		entry->timer_info.hard_timeout = 0;

		//No matches => wildcard on all unmatched frames seen so far in this table

		//No instructions => just execute action set stored for the packet so far

		if (of1x_add_flow_entry_table(table->pipeline, table->number, &entry, false, true) != ROFL_OF1X_FM_SUCCESS){
			return ROFL_FAILURE;
		}
	}

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_egress_vlan_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);
	bitmap256_set(goto_tables, OFDPA_EGRESS_VLAN1_FLOW_TABLE);
	//bitmap256_set(goto_tables, OFDPA_EGRESS_MAINTENANCE_POINT_FLOW_TABLE);

	//no continuation in next table
	table->table_index_next = 0;

	//Set default behaviour MISS drop
	table->default_action = OF1X_TABLE_MISS_DROP;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_ACTSET_OUTPUT);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_VLAN_VID);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_OVID);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_ALLOW_VLAN_TRANSLATION);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_PUSH_VLAN);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_POP_VLAN);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_LMEP_ID);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_OAM_LM_RX_COUNT);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	//bitmap128_set(&table->config.write_actions, OF1X_AT_OFDPA_OAM_LM_TX_Count);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_CLEAR_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


rofl_result_t __ofdpa_set_egress_vlan1_table_defaults(of1x_flow_table_t* table){

	bitmap256_t *goto_tables = &(table->config.goto_tables);

	bitmap256_clean(goto_tables);
	//bitmap256_set(goto_tables, OFDPA_EGRESS_MAINTENANCE_POINT_FLOW_TABLE);

	//no continuation in next table
	table->table_index_next = 0;

	//Set default behaviour MISS drop
	table->default_action = OF1X_TABLE_MISS_DROP;

	//Match
	bitmap128_clean(&table->config.match);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_ACTSET_OUTPUT);
	bitmap128_set(&table->config.match, OF1X_MATCH_VLAN_VID);
	bitmap128_set(&table->config.match, OF1X_MATCH_OFDPA_OVID);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_TYPE);
	bitmap128_set(&table->config.match, OF1X_MATCH_ETH_DST);

	//Wildcards
	bitmap128_clean(&table->config.wildcards);
	bitmap128_set(&table->config.wildcards, OF1X_MATCH_ETH_DST);

	//Apply actions
	bitmap128_clean(&table->config.apply_actions);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_VLAN_VID);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_PUSH_VLAN);
	bitmap128_set(&table->config.apply_actions, OF1X_AT_POP_VLAN);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_LMEP_ID);
	//bitmap128_set(&table->config.apply_actions, OF1X_AT_SET_FIELD_OFDPA_OAM_LM_RX_COUNT);

	//Write actions
	bitmap128_clean(&table->config.write_actions);
	//bitmap128_set(&table->config.write_actions, OF1X_AT_OFDPA_OAM_LM_TX_Count);

	//METADATA (full metadata support)
	table->config.metadata_match = 0x0ULL;
	table->config.metadata_write = 0x0ULL;

	//Instructions
	table->config.instructions =
					(1 << OF1X_IT_APPLY_ACTIONS) |
					(1 << OF1X_IT_CLEAR_ACTIONS) |
					(1 << OF1X_IT_GOTO_TABLE);

	return ROFL_SUCCESS;
}


//
// Snapshots
//

//Creates a snapshot of the running pipeline of an LSI
rofl_result_t __of1x_pipeline_ofdpa_get_snapshot(of1x_pipeline_t* pipeline, of1x_pipeline_snapshot_t* sn){

	int i;
	of1x_flow_table_t* t;

	//Cleanup stuff coming from the cloning process
	sn->sw = NULL;

	//Allocate tables and initialize
	sn->tables = (of1x_flow_table_t*)platform_malloc_shared(sizeof(of1x_flow_table_t)*pipeline->num_of_tables);

	if(!sn->tables)
		return ROFL_FAILURE;

	//Copy contents of the tables and remove references
	memcpy(sn->tables, pipeline->tables, sizeof(of1x_flow_table_t)*pipeline->num_of_tables);

	//Snapshot tables tables
	for(i=0;i<pipeline->num_of_tables;i++){
		__of1x_stats_table_tid_t c;

		t = &sn->tables[i];

		//Consolidate stats
		__of1x_stats_table_consolidate(&t->stats, &c);

		//Memset to 0
		memset(&t->stats,0,sizeof(of1x_stats_table_t));

		//Assign consolidated
		t->stats.s.counters = c;

		t->pipeline = t->rwlock = t->mutex = t->matching_aux[0] = t->matching_aux[1] = NULL;

#if OF1X_TIMER_STATIC_ALLOCATION_SLOTS
#else
		t->timers = NULL;
#endif
	}

	//TODO: deep entry copy?
	sn->num_of_tables = pipeline->num_of_tables;
	sn->num_of_buffers = pipeline->num_of_buffers;
	sn->capabilities = pipeline->capabilities;
	sn->miss_send_len = pipeline->miss_send_len;

	//Allocate GROUPS and initialize
	sn->groups = (of1x_group_table_t*)platform_malloc_shared(sizeof(of1x_group_table_t));

	platform_rwlock_rdlock(pipeline->groups->rwlock);

	//Copy contents (config & num of entries)
	memcpy(sn->groups, pipeline->groups, sizeof(of1x_group_table_t));

	platform_rwlock_rdunlock(pipeline->groups->rwlock);

	//clean unnecessary information
	sn->groups->head = sn->groups->tail = sn->groups->rwlock = NULL;

	return ROFL_SUCCESS;
}

//Destroy a previously getd snapshot
void __of1x_pipeline_ofdpa_destroy_snapshot(of1x_pipeline_snapshot_t* sn){
	//Release tables memory
	platform_free_shared(sn->tables);
	platform_free_shared(sn->groups);
}

//Adjusts a flowmod before being added to a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_ofdpa_flow_add(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, bool check_overlap, bool reset_counts){

	switch (table->number){
	case OFDPA_VLAN_FLOW_TABLE:{
		switch (entry->cookie){
#if 1
		// as done by rofl-ofdpa (TODO: needs proper documentation, btw. is that enum part of OFDPA-2?)
		case OFDPA_FTT_VLAN_VLAN_ASSIGNMENT:{

			/* add a PUSH_VLAN on flow entries of type OFDPA_FTT_VLAN_VLAN_ASSIGNMENT
			 * in table OFDPA_VLAN_FLOW_TABLE, as this is automatically done by
			 * Broadcom ASICs */
			unsigned int i;

			for(i=0;i<OF1X_IT_MAX;i++){
				switch(entry->inst_grp.instructions[i].type){
				case OF1X_IT_APPLY_ACTIONS:{
					of1x_action_group_t *new_ag;
					of1x_action_group_t *old_ag = entry->inst_grp.instructions[i].apply_actions;
					of1x_packet_action_t *it, *action;
					wrap_uint_t field;

					//Create new action_group APPLY_ACTIONS
					if ((new_ag=of1x_init_action_group(0))==NULL){
						return ROFL_OF1X_FM_FAILURE;
					}

					//Action PUSH_VLAN as first action of new_action_group
					field.u16 = ETH_TYPE_VLAN_CTAG_8100;
					if((action=of1x_init_packet_action(OF1X_AT_PUSH_VLAN, field, 0x0))==NULL){
						return ROFL_OF1X_FM_FAILURE;
					}
					of1x_push_packet_action_to_group(new_ag, action);

					//Append remaining packet actions from old_action_group to new_action_group
					for(it=old_ag->head;it;it=it->next){
						action=__of1x_copy_packet_action(it);
						of1x_push_packet_action_to_group(new_ag, action);
					}

					//Replace old action group with new one
					of1x_add_instruction_to_group(&entry->inst_grp, OF1X_IT_APPLY_ACTIONS, new_ag, NULL, NULL, 0);
				}break;
				default:{}
				}
			}
		}break;
#endif
		default:{}
		}
	}break;
	default:{}
	}

	return ROFL_OF1X_FM_SUCCESS;
};

//Adjusts a flowmod before being modified in a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_ofdpa_flow_modify(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, const enum of1x_flow_removal_strictness strict, bool reset_counts){

	switch (table->number){
	case OFDPA_VLAN_FLOW_TABLE:{
		switch (entry->cookie){
#if 1
		// as done by rofl-ofdpa (TODO: needs proper documentation, btw. is that enum part of OFDPA-2?)
		case OFDPA_FTT_VLAN_VLAN_ASSIGNMENT:{

			/* add a PUSH_VLAN on flow entries of type OFDPA_FTT_VLAN_VLAN_ASSIGNMENT
			 * in table OFDPA_VLAN_FLOW_TABLE, as this is automatically done by
			 * Broadcom ASICs */
			unsigned int i;

			for(i=0;i<OF1X_IT_MAX;i++){
				switch(entry->inst_grp.instructions[i].type){
				case OF1X_IT_APPLY_ACTIONS:{
					of1x_action_group_t *new_ag;
					of1x_action_group_t *old_ag = entry->inst_grp.instructions[i].apply_actions;
					of1x_packet_action_t *it, *action;
					wrap_uint_t field;

					//Create new action_group APPLY_ACTIONS
					if ((new_ag=of1x_init_action_group(0))==NULL){
						return ROFL_OF1X_FM_FAILURE;
					}

					//Action PUSH_VLAN as first action of new_action_group
					field.u16 = ETH_TYPE_VLAN_CTAG_8100;
					if((action=of1x_init_packet_action(OF1X_AT_PUSH_VLAN, field, 0x0))==NULL){
						return ROFL_OF1X_FM_FAILURE;
					}
					of1x_push_packet_action_to_group(new_ag, action);

					//Append remaining packet actions from old_action_group to new_action_group
					for(it=old_ag->head;it;it=it->next){
						action=__of1x_copy_packet_action(it);
						of1x_push_packet_action_to_group(new_ag, action);
					}

					//Replace old action group with new one
					of1x_add_instruction_to_group(&entry->inst_grp, OF1X_IT_APPLY_ACTIONS, new_ag, NULL, NULL, 0);
				}break;
				default:{}
				}
			}
		}break;
#endif
		default:{}
		}
	}break;
	default:{}
	}

	return ROFL_OF1X_FM_SUCCESS;
};

//Adjusts a flowmod before being removed from a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_ofdpa_flow_remove(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t* entry, const enum of1x_flow_removal_strictness strict, uint32_t out_port, uint32_t out_group){
	return ROFL_OF1X_FM_SUCCESS;
};

//Adjusts a groupmod before being added to group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_ofdpa_group_add(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets){
	return ROFL_OF1X_GM_SUCCESS;
};

//Adjusts a groupmod before being modified in group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_ofdpa_group_modify(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets){
	return ROFL_OF1X_GM_SUCCESS;
};

//Adjusts a groupmod before being removed from group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_ofdpa_group_delete( struct of1x_pipeline *pipeline, of1x_group_table_t *gt, uint32_t id){
	return ROFL_OF1X_GM_SUCCESS;
};

