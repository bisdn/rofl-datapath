/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __DATAPACKET_H__
#define __DATAPACKET_H__

#include <stdbool.h>
#include <sys/time.h>
#include "../openflow/of_switch.h"

//OF1.X
#include "../openflow/openflow1x/pipeline/of1x_action.h"
//Add more here...

/**
* @file datapacket.h
* @author Marc Sune<marc.sune (at) bisdn.de>
* 
* @brief Defines the common packet abstraction that 
* a logical switch can process through its pipeline.
*
* The datapacket is an OF version agnostic data packet abstraction
* which may also contain platform specific state (likely at least a
* reference to the packet buffer in the platform). 
*
*/

/* Write actions */
typedef union of_write_actions{
	//OF1.X
	of1x_write_actions_t of1x;
	//Add more here...	
}of_write_actions_t;

//Typedef to void. This is dependant to the version of the pipeline
typedef void platform_datapacket_state_t; 

/**
* @brief Data packet abstraction
*
* Abstraction that represents a data packet that may transverse
* one Logical Switch OpenFlow Pipeline. This abstraction is OpenFlow 
* version agnostic. It contains a (void*) reference, platform_state
* to allow the user of the library to keep platform specific state 
* while transversing the pipeline, which the packet mangling APIs may
* use afterwards.
*/
typedef struct datapacket{

	//Packet identifier
	uint64_t id;

	//Pointer to the switch which is processing the packet
	of_switch_t const* sw;

	//Generic OpenFlow metadata and write actions 
	uint64_t __metadata;	
	of_write_actions_t write_actions;
	
	//OpenFlow 1.3 cookie
	uint64_t __cookie;

	//OpenFlow 1.3 tunnel_id
	uint64_t __tunnel_id;

	//OFDPA VRF
	uint16_t __vrf;

	//OFDPA OVID
	uint16_t __ovid;

	//OFDPA ALlow VLAN Translation
	uint8_t __allow_vlan_translation;

	//OFDPA Action_Set_Output
	uint32_t __action_set_output_egress_portno;

	/**
	* Flag indicating if it is a replica of the original packet
	* (used for multi-output matches)
	*/
	bool is_replica;
	
	/** 
	* @brief Platform specific state. 
	* 
	* This is not OF related state and platform  specific, and may be
	* used by the library user to keep platform specific state.
	* This may typically be, at least, a reference to the packet
	* buffer in the platform.
	*/
	platform_datapacket_state_t* platform_state;

}datapacket_t;

static inline void __init_packet_metadata(datapacket_t *const pkt){
	pkt->__metadata = 0ULL;
	pkt->__cookie = 0ULL;
	pkt->__tunnel_id = 0ULL;
	pkt->__vrf = 0;
	pkt->__ovid = 0;
	pkt->__allow_vlan_translation = 0;
	pkt->__action_set_output_egress_portno = 0;
};

#endif //DATAPACKET

