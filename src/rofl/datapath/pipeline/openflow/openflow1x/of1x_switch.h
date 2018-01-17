/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __OF1X_SWITCH_H__
#define __OF1X_SWITCH_H__

#include <stdlib.h>
#include <string.h>
#include "rofl_datapath.h"
#include "../of_switch.h"
#include "pipeline/of1x_pipeline.h"

/**
* @file of1x_switch.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief OpenFlow v1.0, 1.2 and 1.3.2 logical switch abstraction
*
*/

#define OF1XP_NO_BUFFER	0xffffffff

/**
* @ingroup core_of1x 
* OpenFlow-enabled v1.0, 1.2 and 1.3.2 switch abstraction
*/
typedef struct of1x_switch{
	
	//General switch instance information
	
	/* This part is common and MUST be at the very beginning */ 
	of_version_t of_ver;
	sw_flavor_t sw_flavor;
	uint64_t dpid;
	char name[LOGICAL_SWITCH_MAX_LEN_NAME];
	unsigned int max_ports;
	unsigned int num_of_ports;
	
	//Switch logical ports 
	logical_switch_port_t logical_ports[LOGICAL_SWITCH_MAX_LOG_PORTS];
 	
	//Platform agnostic pointer
	of_switch_platform_state_t* platform_state;
	/* End of common part */

	//pipeline
	of1x_pipeline_t pipeline;
	
	//Mutex
	platform_mutex_t* mutex;

}of1x_switch_t;

/**
* Switch snapshot
*/
typedef of1x_switch_t of1x_switch_snapshot_t;

//C++ extern C
ROFL_BEGIN_DECLS

/* Initializer and destroyer */

/**
* @brief Creates an OpenFlow v1.0, 1.2 and 1.3.2 forwarding instance.  
* @ingroup core_of1x 
* @param version OF version 
* @param num_of_tables Number of tables that the v1.2 and 1.3 pipeline should have. This is immutable 
* during the lifetime of the switch.
* @param ma_list An array with num_of_tables, with the matching algorithm that should
* be used in each table (0..num_of_tables-1) 
*/
of1x_switch_t* of1x_init_switch(const char* name, of_version_t version, sw_flavor_t flavor, uint64_t dpid, unsigned int num_of_tables, enum of1x_matching_algorithm_available* ma_list);

/* Reconfigures the pipeline to behave as an OF specific version pipeline. Warning: this function may DELETE all the entries in the tables, timers and group entries of the switch */
rofl_result_t __of1x_reconfigure_switch(of1x_switch_t* sw, of_version_t version);

rofl_result_t __of1x_destroy_switch(of1x_switch_t* sw);

/* Port management */
rofl_result_t __of1x_attach_port_to_switch_at_port_num(of1x_switch_t* sw, unsigned int port_num, switch_port_t* port);
rofl_result_t __of1x_attach_port_to_switch(of1x_switch_t* sw, switch_port_t* port, unsigned int* port_num);
rofl_result_t __of1x_detach_port_from_switch_by_port_num(of1x_switch_t* sw, unsigned int port_num);
rofl_result_t __of1x_detach_port_from_switch(of1x_switch_t* sw, switch_port_t* port);
rofl_result_t __of1x_detach_all_ports_from_switch(of1x_switch_t* sw);

/* Dump */
/**
* @brief Dumps the OpenFlow v1.0, 1.2 and 1.3.2 forwarding instance, for debugging purposes.  
* @ingroup core_of1x 
*
* @param nbo Show values in network byte order (ignored in BIG ENDIAN systems).
*/
void of1x_dump_switch(of1x_switch_t* sw, bool nbo);
/**
* @brief Dumps the OpenFlow v1.0, 1.2 and 1.3.2 forwarding instance, for debugging purposes.  
* @ingroup core_of1x 
*
* @param nbo Show values in network byte order (ignored in BIG ENDIAN systems).
*/
void of1x_full_dump_switch(of1x_switch_t* sw, bool nbo);

//Creates a snapshot of the running of LSI 
of1x_switch_snapshot_t* __of1x_switch_get_snapshot(of1x_switch_t* sw);

//Destroy a previously generated snapshot
void __of1x_switch_destroy_snapshot(of1x_switch_snapshot_t* snapshot);


//C++ extern C
ROFL_END_DECLS

#endif //OF1X_SWITCH
