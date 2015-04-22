#include "rofl_datapath.h"

#ifndef ROFL_BUILD
	//No git detected
	const char ROFL_DATAPATH_VERSION[]=ROFL_DATAPATH_VERSION_;
	const char ROFL_DATAPATH_BUILD_NUM[]="";
	const char ROFL_DATAPATH_BUILD_BRANCH[]="";
	const char ROFL_DATAPATH_BUILD_DESCRIBE[]="";
#else
	const char ROFL_DATAPATH_VERSION[]=ROFL_DATAPATH_VERSION_;
	const char ROFL_DATAPATH_BUILD_NUM[]=ROFL_DATAPATH_BUILD;
	const char ROFL_DATAPATH_BUILD_BRANCH[]=ROFL_DATAPATH_BRANCH;
	const char ROFL_DATAPATH_BUILD_DESCRIBE[]=ROFL_DATAPATH_DESCRIBE;

#endif

//C++ extern C
ROFL_BEGIN_DECLS

// autoconf AC_CHECK_LIB helper function as C-declaration
void librofl_datapath_is_present(void) {};

//C++ extern C
ROFL_END_DECLS


