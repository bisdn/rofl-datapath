/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
* @file threading_pp.h
* @author Marc Sune<marc.sune (at) bisdn.de>
*
* @brief Threading Packet Processing API utils 
*/

#ifndef __THREADING_PP_H__
#define __THREADING_PP_H__

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "rofl_datapath.h"
#include "platform/likely.h"
#include "util/cpu_cache.h"

#if !defined(__GNUC__) && !defined(__INTEL_COMPILER)
	#error Unknown compiler; could not guess which compare-and-swap instructions to use
#else
	#define CAS(ptr, oldval, newval) __sync_bool_compare_and_swap(ptr, oldval, newval)
	#define tid_memory_barrier __sync_synchronize
#endif

//Present flag
typedef struct{
	uint64_t flag;
}__pipeline_cache_aligned tid_presence_it_t;
ROFL_PIPELINE_CHECK_CACHE_ALIGNED(tid_presence_it_t);

//Presence flags
typedef struct{
	tid_presence_it_t flags[ROFL_PIPELINE_MAX_TIDS];
}__pipeline_cache_aligned tid_presence_t;
ROFL_PIPELINE_CHECK_CACHE_ALIGNED(tid_presence_t);

static inline void tid_init_presence_mask(tid_presence_t* presence_mask){
	memset(presence_mask, 0, sizeof(tid_presence_t));
}

/**
* Set thread presence
*/
static inline void tid_mark_as_present(unsigned int tid, volatile tid_presence_t* presence_mask){
	assert(tid < ROFL_PIPELINE_MAX_TIDS);
	presence_mask->flags[tid].flag = 0x1UL;
}

/**
* Unset thread presence
*/
static inline void tid_mark_as_not_present(unsigned int tid, volatile tid_presence_t* presence_mask){
	presence_mask->flags[tid].flag = 0x0UL;
}

/**
*
*/
static inline void tid_wait_all_not_present(volatile tid_presence_t* presence_mask){
	int i;

	//Memory barrier first
	tid_memory_barrier();

	for(i=0;i<ROFL_PIPELINE_MAX_TIDS;i++)
		while(presence_mask->flags[i].flag == 0x1UL);
}

#endif //THREADING_PP
