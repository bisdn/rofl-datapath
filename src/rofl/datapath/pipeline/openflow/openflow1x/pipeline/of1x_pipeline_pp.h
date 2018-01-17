/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __OF1X_PIPELINE_PP_H__
#define __OF1X_PIPELINE_PP_H__

#include "rofl_datapath.h"
#include "../../../util/pp_guard.h" //Never forget to include the guard
#include "../of1x_switch.h"
#include "of1x_pipeline.h"
#include "of1x_flow_table_pp.h"
#include "of1x_instruction_pp.h"
#include "of1x_statistics_pp.h"
#include "flavors/of1x_pipeline_generic_pp.h"
#include "flavors/of1x_pipeline_ofdpa_pp.h"

//This block is not necessary but it is useful to prevent
//double definitions of static inline methods
#include "of1x_action.h"
#include "of1x_instruction.h"
#include "of1x_flow_table.h"
#include "of1x_group_table.h"
#include "of1x_timers.h"

//Platform stuff
#include "../../../platform/lock.h"
#include "../../../platform/likely.h"
#include "../../../platform/memory.h"
#include "../../../platform/packet.h"
#include "../../../platform/atomic_operations.h"
#include "../of1x_async_events_hooks.h"

#include "matching_algorithms/available_ma_pp.h"

#include "../../../util/logging.h"


/**
* @file of1x_pipeline_pp.h
* @author Marc Sune<marc.sune (at) bisdn.de>
* @author Andreas Koepsel <andreas.koepsel (at) bisdn.de>
*
* @brief OpenFlow v1.0, 1.2 and 1.3.2 pipeline packet processing routines
*/

//C++ extern C
ROFL_BEGIN_DECLS


/*
* Packet processing through pipeline
*
*/
static inline void __of1x_process_packet_pipeline(const unsigned int tid, const of_switch_t *sw, datapacket_t *const pkt){

	if (sw == NULL)
		return;
	switch (sw->sw_flavor){
	case SW_FLAVOR_GENERIC:{
		__of1x_process_packet_pipeline_generic(tid, sw, pkt);
	}break;
	case SW_FLAVOR_OFDPA:{
		__of1x_process_packet_pipeline_ofdpa(tid, sw, pkt);
	}break;
	default:{
		assert(0);
	}
	}
}

/**
* @brief Processes a packet-out through the OpenFlow pipeline.  
* @ingroup core_pp 
*/
static inline void of1x_process_packet_out_pipeline(const unsigned int tid, const of1x_switch_t *sw, datapacket_t *const pkt, const of1x_action_group_t* apply_actions_group){
	
	if(sw == NULL)
		return;
	switch (sw->sw_flavor){
	case SW_FLAVOR_GENERIC:{
		__of1x_process_packet_out_pipeline_generic(tid, sw, pkt, apply_actions_group);
	}break;
	case SW_FLAVOR_OFDPA:{
		__of1x_process_packet_out_pipeline_ofdpa(tid, sw, pkt, apply_actions_group);
	}break;
	default:{
		assert(0);
	}
	}
}

//C++ extern C
ROFL_END_DECLS

#endif //OF1X_PIPELINE_PP
