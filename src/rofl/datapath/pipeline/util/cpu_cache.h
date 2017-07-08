/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ROFL_PIPELINE_CPU_CACHE_H_
#define ROFL_PIPELINE_CPU_CACHE_H_

#include "rofl_datapath.h"
#include "compile_assert.h"

/**
* @author Marc Sune <marcdevel (at) gmail.com>
* @brief Defines useful MACROS for CPU cache alignment
*/

//Padd structs to align cache
#ifdef ROFL_PIPELINE_CACHE_ALIGNED
	#define __pipeline_cache_aligned \
			__attribute__((__aligned__( ROFL_PIPELINE_CACHE_LINE )))
	#define ROFL_PIPELINE_CHECK_CACHE_ALIGNED(T) \
			enum {  COMPILATION_ASSERT_NOT_CACHE_ALIGNED_ ## T = 1/( ((sizeof( T ) % ROFL_PIPELINE_CACHE_LINE) == 0)) }
#else
	#define __pipeline_cache_aligned
	#define ROFL_PIPELINE_CHECK_CACHE_ALIGNED(T)
#endif //ROFL_PIPELINE_CACHE_ALIGNED

#endif //ROFL_PIPELINE_CPU_CACHE
