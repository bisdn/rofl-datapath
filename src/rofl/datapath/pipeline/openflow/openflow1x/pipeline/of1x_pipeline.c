#include "of1x_pipeline.h"
#include "of1x_instruction.h"
#include "of1x_flow_table.h"
#include "of1x_group_table.h"
#include "of1x_timers.h"
#include "../../../platform/lock.h"
#include "../../../platform/likely.h"
#include "../../../platform/memory.h"
#include "../../../platform/atomic_operations.h"
#include "../of1x_async_events_hooks.h"
#include "matching_algorithms/matching_algorithms.h"
#include "flavors/of1x_pipeline_generic.h"
#include "flavors/of1x_pipeline_ofdpa.h"
#include "../of1x_switch.h"

#include "../../../util/logging.h"

/* 
* This file implements the abstraction of a pipeline
*/

/* Management operations */
rofl_result_t __of1x_init_pipeline(struct of1x_switch* sw, const unsigned int num_of_tables, enum of1x_matching_algorithm_available* list){

	if(!sw)
		return ROFL_FAILURE;

	of1x_pipeline_t* pipeline = &sw->pipeline;

	//Fill in back reference
	pipeline->sw = sw;

	switch (sw->sw_flavor){
	case SW_FLAVOR_GENERIC:
		return __of1x_init_pipeline_generic(pipeline, num_of_tables, list);
	case SW_FLAVOR_OFDPA:
		return __of1x_init_pipeline_ofdpa(pipeline, num_of_tables, list);
	default:{
		assert(0);
	}
	}
	return ROFL_SUCCESS;
}

rofl_result_t __of1x_destroy_pipeline(of1x_pipeline_t* pipeline){
	
	if(!pipeline)
		return ROFL_FAILURE;

	switch (pipeline->sw->sw_flavor) {
	case SW_FLAVOR_GENERIC:
		return __of1x_destroy_pipeline_generic(pipeline);
	case SW_FLAVOR_OFDPA:
		return __of1x_destroy_pipeline_ofdpa(pipeline);
	default:{
		assert(0);
	}
	}
	return ROFL_SUCCESS;
}

//Purge of all entries in the pipeline (reset)	
rofl_result_t __of1x_purge_pipeline_entries(of1x_pipeline_t* pipeline){

	if(!pipeline)
		return ROFL_FAILURE;

	switch (pipeline->sw->sw_flavor) {
	case SW_FLAVOR_GENERIC:
		return __of1x_purge_pipeline_generic_entries(pipeline);
	case SW_FLAVOR_OFDPA:
		return __of1x_purge_pipeline_ofdpa_entries(pipeline);
	default:{
		assert(0);
	}
	}
	return ROFL_SUCCESS;
}

//Set the default tables(flow and group tables) configuration according to the new version
rofl_result_t __of1x_set_pipeline_tables_defaults(of1x_pipeline_t* pipeline, of_version_t version){

	if(!pipeline)
		return ROFL_FAILURE;

	switch (pipeline->sw->sw_flavor) {
	case SW_FLAVOR_GENERIC:
		return __of1x_set_pipeline_generic_tables_defaults(pipeline, version);
	case SW_FLAVOR_OFDPA:
		return __of1x_set_pipeline_ofdpa_tables_defaults(pipeline, version);
	default:{
		assert(0);
	}
	}
	return ROFL_SUCCESS;
}



//
// Snapshots
//

//Creates a snapshot of the running pipeline of an LSI 
rofl_result_t __of1x_pipeline_get_snapshot(of1x_pipeline_t* pipeline, of1x_pipeline_snapshot_t* sn){

	if(!pipeline)
		return ROFL_FAILURE;

	switch (pipeline->sw->sw_flavor) {
	case SW_FLAVOR_GENERIC:
		return __of1x_pipeline_generic_get_snapshot(pipeline, sn);
	case SW_FLAVOR_OFDPA:
		return __of1x_pipeline_ofdpa_get_snapshot(pipeline, sn);
	default:{
		assert(0);
	}
	}
	return ROFL_SUCCESS;
}

//Destroy a previously retrieved snapshot
void __of1x_pipeline_destroy_snapshot(of1x_pipeline_snapshot_t* sn){

	if(!sn)
		return;

	if (sn->sw){
		switch (sn->sw->sw_flavor) {
		case SW_FLAVOR_GENERIC: {
			__of1x_pipeline_generic_destroy_snapshot(sn);
		}  break;
		case SW_FLAVOR_OFDPA: {
			__of1x_pipeline_ofdpa_destroy_snapshot(sn);
		} break;
		default:{
			assert(0);
		}
		}
	}else{
		if (sn->tables)
			platform_free_shared(sn->tables);
		if (sn->groups)
			platform_free_shared(sn->groups);
	}
}


