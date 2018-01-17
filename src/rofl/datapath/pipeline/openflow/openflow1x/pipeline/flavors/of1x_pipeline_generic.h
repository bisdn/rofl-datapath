/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __OF1X_PIPELINE_GENERIC_H__
#define __OF1X_PIPELINE_GENERIC_H__

#include <stdlib.h>
#include "rofl_datapath.h"
#include "../of1x_pipeline.h"
#include "../of1x_flow_table.h"
#include "../of1x_group_table.h"
#include "../../../../common/bitmap.h"
#include "../../../../common/datapacket.h"
#include "../../../of_switch.h"

//C++ extern C
ROFL_BEGIN_DECLS

/* Pipeline mgmt */
rofl_result_t __of1x_init_pipeline_generic(struct of1x_pipeline* pipeline, const unsigned int num_of_tables, enum of1x_matching_algorithm_available* list);
rofl_result_t __of1x_destroy_pipeline_generic(of1x_pipeline_t* pipeline);

//Purge of all entries in the pipeline (reset)
rofl_result_t __of1x_purge_pipeline_generic_entries(of1x_pipeline_t* pipeline);

//Set the default tables(flow and group tables) configuration according to the new version
rofl_result_t __of1x_set_pipeline_generic_tables_defaults(of1x_pipeline_t* pipeline, of_version_t version);

//
// Snapshots
//

//Creates a snapshot of the running pipeline of an LSI
rofl_result_t __of1x_pipeline_generic_get_snapshot(of1x_pipeline_t* pipeline, of1x_pipeline_snapshot_t* snapshot);

//Destroy a previously generated snapshot
void __of1x_pipeline_generic_destroy_snapshot(of1x_pipeline_snapshot_t* snapshot);

//
// pipeline operations
//

//Adjusts a flowmod before being added to a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_generic_flow_add(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, bool check_overlap, bool reset_counts);
//Adjusts a flowmod before being modified in a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_generic_flow_modify(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, const enum of1x_flow_removal_strictness strict, bool reset_counts);
//Adjusts a flowmod before being removed from a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_generic_flow_remove(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, const enum of1x_flow_removal_strictness strict, uint32_t out_port, uint32_t out_group);

//Adjusts a groupmod before being added to group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_generic_group_add(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets);
//Adjusts a groupmod before being modified in group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_generic_group_modify(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets);
//Adjusts a groupmod before being removed from group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_generic_group_delete( struct of1x_pipeline *pipeline, of1x_group_table_t *gt, uint32_t id);


//C++ extern C
ROFL_END_DECLS

#endif //OF1X_PIPELINE_GENERIC
