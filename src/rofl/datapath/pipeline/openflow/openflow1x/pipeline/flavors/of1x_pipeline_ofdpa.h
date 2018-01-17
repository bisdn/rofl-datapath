/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __OF1X_PIPELINE_OFDPA_H__
#define __OF1X_PIPELINE_OFDPA_H__

#include <stdlib.h>
#include "rofl_datapath.h"
#include "../of1x_pipeline.h"
#include "../of1x_flow_table.h"
#include "../of1x_group_table.h"
#include "../../../../common/bitmap.h"
#include "../../../../common/datapacket.h"
#include "../../../of_switch.h"
#include "../of1x_ofdpa_datatypes.h"

enum OFDPA_FLOW_TABLE_ID_FMT_VLAN {
  OFDPA_FTT_VLAN_VLAN_FILTERING = (0ULL << (7*8)),
  OFDPA_FTT_VLAN_VLAN_ASSIGNMENT = (1ULL << (7*8)),
  OFDPA_FTT_VLAN_VLAN_ALLOW_ALL = (2ULL << (7*8)),
  OFDPA_FTT_VLAN_VLAN_TRANSLATE_SINGLE_TAG_OR_SINGLE_TAG_TO_DOUBLE_TAG = (3ULL << (7*8)),
  OFDPA_FTT_VLAN_VLAN_TRANSLATE_DOUBLE_TAG_TO_SINGLE_TAG = (4ULL << (7*8)),
  OFDPA_FTT_VLAN_MPLS_L2_SINGLE_TAG = (5ULL << (7*8)),
  OFDPA_FTT_VLAN_MPLS_L2_DOUBLE_TAG = (6ULL << (7*8)),
  OFDPA_FTT_VLAN_MPLS_L2_ALL_TRAFFIC_ON_PORT = (7ULL << (7*8)),
  OFDPA_FTT_VLAN_ETHERNET_LINK_OAM = (8ULL << (7*8)),
  OFDPA_FTT_VLAN_ETHERNET_LINK_OAM_LOOPBACK = (9ULL << (7*8)),
};

/**
 * auxiliary structure binding OFDPA table and associated matching algorithm
 */
typedef struct ofdpa_table_setting{
	unsigned int table_number;
	enum of1x_matching_algorithm_available ma_alg;
}ofdpa_table_setting_t;

//C++ extern C
ROFL_BEGIN_DECLS

/* Pipeline mgmt */
rofl_result_t __of1x_init_pipeline_ofdpa(struct of1x_pipeline* pipeline, const unsigned int num_of_tables, enum of1x_matching_algorithm_available* list);
rofl_result_t __of1x_destroy_pipeline_ofdpa(of1x_pipeline_t* pipeline);

//Purge of all entries in the pipeline (reset)
rofl_result_t __of1x_purge_pipeline_ofdpa_entries(of1x_pipeline_t* pipeline);

//Set the default tables(flow and group tables) configuration according to the new version
rofl_result_t __of1x_set_pipeline_ofdpa_tables_defaults(of1x_pipeline_t* pipeline, of_version_t version);

//Set OFDPA table defaults
rofl_result_t __ofdpa_set_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_ingress_port_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_vlan_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_vlan1_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_termination_mac_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_bridging_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_unicast_routing_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_multicast_routing_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_policy_acl_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_egress_vlan_table_defaults(of1x_flow_table_t* table);
rofl_result_t __ofdpa_set_egress_vlan1_table_defaults(of1x_flow_table_t* table);

//
// Snapshots
//

//Creates a snapshot of the running pipeline of an LSI
rofl_result_t __of1x_pipeline_ofdpa_get_snapshot(of1x_pipeline_t* pipeline, of1x_pipeline_snapshot_t* snapshot);

//Destroy a previously generated snapshot
void __of1x_pipeline_ofdpa_destroy_snapshot(of1x_pipeline_snapshot_t* snapshot);


//
// pipeline operations
//

//Adjusts a flowmod before being added to a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_ofdpa_flow_add(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, bool check_overlap, bool reset_counts);
//Adjusts a flowmod before being modified in a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_ofdpa_flow_modify(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t *entry, const enum of1x_flow_removal_strictness strict, bool reset_counts);
//Adjusts a flowmod before being removed from a flow table if needed
rofl_of1x_fm_result_t __of1x_pipeline_ofdpa_flow_remove(struct of1x_pipeline *const pipeline, of1x_flow_table_t *table, of1x_flow_entry_t* entry, const enum of1x_flow_removal_strictness strict, uint32_t out_port, uint32_t out_group);

//Adjusts a groupmod before being added to group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_ofdpa_group_add(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets);
//Adjusts a groupmod before being modified in group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_ofdpa_group_modify(of1x_group_table_t *gt, of1x_group_type_t type, uint32_t id, of1x_bucket_list_t **buckets);
//Adjusts a groupmod before being removed from group table if needed
rofl_of1x_gm_result_t __of1x_pipeline_ofdpa_group_delete( struct of1x_pipeline *pipeline, of1x_group_table_t *gt, uint32_t id);


//C++ extern C
ROFL_END_DECLS

#endif //OF1X_PIPELINE_OFDPA
