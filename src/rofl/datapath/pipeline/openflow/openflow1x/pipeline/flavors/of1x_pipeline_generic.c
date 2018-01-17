#include "of1x_pipeline_generic.h"
#include "../of1x_pipeline.h"
#include "../of1x_instruction.h"
#include "../of1x_flow_table.h"
#include "../of1x_group_table.h"
#include "../of1x_timers.h"
#include "../../../../platform/lock.h"
#include "../../../../platform/likely.h"
#include "../../../../platform/memory.h"
#include "../../../../platform/atomic_operations.h"
#include "../../of1x_async_events_hooks.h"
#include "../matching_algorithms/matching_algorithms.h"
#include "../../of1x_switch.h"

#include "../../../../util/logging.h"

rofl_result_t __of1x_init_pipeline_generic(struct of1x_pipeline* pipeline, const unsigned int num_of_tables, enum of1x_matching_algorithm_available* list){

	int i,j;

	//Verify params
	if( ! (num_of_tables <= OF1X_MAX_FLOWTABLES && num_of_tables > 0) )
		return ROFL_FAILURE;

	//Fill in
	pipeline->num_of_tables = num_of_tables;
	pipeline->num_of_buffers = 0; //Should be filled in the post_init hook
	pipeline->first_table_index = OF1X_FIRST_FLOW_TABLE_INDEX;

	pipeline->ops.pre_flow_add_hook     = &__of1x_pipeline_generic_flow_add;
	pipeline->ops.pre_flow_modify_hook  = &__of1x_pipeline_generic_flow_modify;
	pipeline->ops.pre_flow_delete_hook  = &__of1x_pipeline_generic_flow_remove;
	pipeline->ops.pre_group_add_hook    = &__of1x_pipeline_generic_group_add;
	pipeline->ops.pre_group_modify_hook = &__of1x_pipeline_generic_group_modify;
	pipeline->ops.pre_group_delete_hook = &__of1x_pipeline_generic_group_delete;

	//Allocate tables and initialize
	pipeline->tables = (of1x_flow_table_t*)platform_malloc_shared(sizeof(of1x_flow_table_t)*num_of_tables);

	if(!pipeline->tables){
		return ROFL_FAILURE;
	}

	for(i=0;i<num_of_tables;i++){

		int table_index_next = (i == (num_of_tables - 1)) ? 0 : i + 1;

		bitmap256_t goto_tables;
		bitmap256_clean(&goto_tables);
		//for the generic pipeline all tables with (table_id > i) are valid goto targets
		for (j=i+1;j<num_of_tables;j++){
			bitmap256_set(&goto_tables, j);
		}

		if( (list[i] >= of1x_matching_algorithm_count) ||
			(__of1x_init_table(pipeline, &pipeline->tables[i], /*table_number=*/i, /*table_index=*/i, table_index_next, goto_tables, list[i]) != ROFL_SUCCESS)
		){
			ROFL_PIPELINE_ERR("Creation of table #%d has failed in logical switch %s. This might be due to an invalid Matching Algorithm or that the system has run out of memory. Aborting Logical Switch creation\n",i,pipeline->sw->name);
			//Destroy already allocated tables
			for(--i; i>=0; i--){
				__of1x_destroy_table(&pipeline->tables[i]);
			}

			platform_free_shared(pipeline->tables);
			return ROFL_FAILURE;
		}
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

	return ROFL_SUCCESS;
}

rofl_result_t __of1x_destroy_pipeline_generic(of1x_pipeline_t* pipeline){

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
rofl_result_t __of1x_purge_pipeline_generic_entries(of1x_pipeline_t* pipeline){

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
rofl_result_t __of1x_set_pipeline_generic_tables_defaults(of1x_pipeline_t* pipeline, of_version_t version){

	unsigned int i;

	//Reset table defaults according to specific version
	for(i=OF1X_FIRST_FLOW_TABLE_INDEX; i < pipeline->num_of_tables ; i++){
		switch(version){
			case OF_VERSION_10:
				__of10_set_table_defaults(&pipeline->tables[i]);
				break;
			case OF_VERSION_12:
				__of12_set_table_defaults(&pipeline->tables[i]);
				__of12_set_group_table_defaults(pipeline->groups);
				break;
			case OF_VERSION_13:
				__of13_set_table_defaults(&pipeline->tables[i]);
				__of13_set_group_table_defaults(pipeline->groups);
				break;
			default:
				return ROFL_FAILURE;

		}
	}

	return ROFL_SUCCESS;
}



//
// Snapshots
//

//Creates a snapshot of the running pipeline of an LSI
rofl_result_t __of1x_pipeline_generic_get_snapshot(of1x_pipeline_t* pipeline, of1x_pipeline_snapshot_t* sn){

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
void __of1x_pipeline_generic_destroy_snapshot(of1x_pipeline_snapshot_t* sn){
	//Release tables memory
	platform_free_shared(sn->tables);
	platform_free_shared(sn->groups);
}

//Adjusts a flowmod before being added to a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_generic_flow_add(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, bool check_overlap, bool reset_counts){
	return ROFL_OF1X_FM_SUCCESS;
};

//Adjusts a flowmod before being modified in a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_generic_flow_modify(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, const enum of1x_flow_removal_strictness strict, bool reset_counts){
	return ROFL_OF1X_FM_SUCCESS;
};

//Adjusts a flowmod before being removed from a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_generic_flow_remove(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, const enum of1x_flow_removal_strictness strict, uint32_t out_port, uint32_t out_group){
	return ROFL_OF1X_FM_SUCCESS;
};

//Adjusts a groupmod before being added to group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_generic_group_add(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets){
	return ROFL_OF1X_GM_SUCCESS;
};

//Adjusts a groupmod before being modified in group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_generic_group_modify(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets){
	return ROFL_OF1X_GM_SUCCESS;
};

//Adjusts a groupmod before being removed from group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_generic_group_delete( struct of1x_pipeline *pipeline, of1x_group_table_t *gt, uint32_t id){
	return ROFL_OF1X_GM_SUCCESS;
};
