/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef HAL_H
#define HAL_H

/**
* @file hal.h
* @author Marc Sune<marc.sune (at) bisdn.de>
* @author Andreas Koepsel<andreas.koepsel (at) bisdn.de>
*
* @brief Hardware Abstraction Layer (HAL) main header file
*/

#include <assert.h>
#include <stdlib.h>
#include <rofl_datapath.h>

#include "hal_utils.h"

/**
* HAL general return codes
*/
typedef enum hal_result {
	HAL_SUCCESS	= ROFL_SUCCESS,
	HAL_FAILURE	= ROFL_FAILURE,
}hal_result_t;

#endif /* HAL_H */
