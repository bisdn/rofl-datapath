/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ROFL_PIPELINE_COMPILE_ASSERT_H_
#define ROFL_PIPELINE_COMPILE_ASSERT_H_

/**
* Marc Sune<marcdevel at gmail dot com>
*/

//Compilation assert
#define COMPILATION_ASSERT(tag,cond) \
	enum { COMPILATION_ASSERT__ ## tag = 1/(cond) }

#endif /* ROFL_PIPELINE_COMPILE_ASSERT_H_ */
