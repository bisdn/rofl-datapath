/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __OF1X_OFDPA_DATATYPESH__
#define __OF1X_OFDPA_DATATYPESH__

/**
* @file of1x_ofdpa_datatypes.h
* @author Andreas Koepsel<andreas.koepsel (at) bisdn.de>
*
* @brief OFDPA 2.0 related datatypes
*
*/

/**
* flow tables defined by OFDPA reference model (OF-DPA Specs v2, Section 6.4 "Table Numbering")
*/
typedef enum{
	OFDPA_INGRESS_PORT_FLOW_TABLE = 0,               /* Ingress Port Flow Table */
	OFDPA_VLAN_FLOW_TABLE = 10,                      /* VLAN Flow Table */
	OFDPA_VLAN1_FLOW_TABLE = 11,                     /* VLAN1 Flow Table */
	OFDPA_MPLS_L2_PORT_FLOW_TABLE = 13,              /* MPLS L2 Port Flow Table */
	OFDPA_TERMINATION_MAC_FLOW_TABLE = 20,           /* Termination MAC Flow Table */
	OFDPA_MPLS0_FLOW_TABLE = 23,                     /* MPLS0 FLow Table */
	OFDPA_MPLS1_FLOW_TABLE = 24,                     /* MPLS1 FLow Table */
	OFDPA_MPLS2_FLOW_TABLE = 25,                     /* MPLS2 FLow Table */
	OFDPA_MAINTENANCE_POINT_FLOW_TABLE = 26,         /* Maintenance Point Flow Table */
	OFDPA_UNICAST_ROUTING_FLOW_TABLE = 30,           /* Unicast Routing Flow Table */
	OFDPA_MULTICAST_ROUTING_FLOW_TABLE = 40,         /* Multicast Routing Flow Table */
	OFDPA_BRIDGING_FLOW_TABLE = 50,                  /* Bridging Flow Table */
	OFDPA_POLICY_ACL_FLOW_TABLE = 60,                /* Policy ACL Flow Table */
	OFDPA_COLOR_BASED_ACTIONS_FLOW_TABLE = 65,       /* Color Based Actions Flow Table */
	OFDPA_EGRESS_VLAN_FLOW_TABLE = 210,              /* Egress VLAN Flow Table */
	OFDPA_EGRESS_VLAN1_FLOW_TABLE = 211,             /* Egress VLAN1 Flow Table */
	OFDPA_EGRESS_MAINTENANCE_POINT_FLOW_TABLE = 226, /* Egress Maintenance Point Flow Table */
	OFDPA_SOURCE_MAC_LEARNING_FLOW_TABLE = 254,      /* Source MAC Learning Flow Table */
	// max table_id used by OFDPA
	OFDPA_MAX_FLOW_TABLE,
}ofdpa_flow_table_t;

#endif //OF1X_OFDPA_DATATYPES
