
##
## Pipeline optimizations
## * Check inline platform functions
## * Check ROFL_PIPELINE_MAX_TIDS
##
##
#if test "$PIPELINE_SUPPORT" = "yes"; then
CFLAGS+=" -D__COMPILING_ROFL_PIPELINE__"
CXXFLAGS+=" -D__COMPILING_ROFL_PIPELINE__"

AC_ARG_WITH([pipeline-platform-funcs-inlined], AS_HELP_STRING([--with-pipeline-platform-funcs-inlined], [Inline platform functions in ROFL-pipeline packet processing API [default=no]]))

AC_MSG_CHECKING(whether to inline platform functions in ROFL-pipeline packet processing API)
AS_IF([test "x$with_pipeline_platform_funcs_inlined" == xyes],[
	AC_SUBST([ROFL_PIPELINE_INLINE_PP_PLATFORM_FUNCS], ["#define ROFL_PIPELINE_INLINE_PP_PLATFORM_FUNCS 1"])
	AC_SUBST([ROFL_PIPELINE_ABORT_IF_INLINED], ["
#ifdef ROFL_PIPELINE_ABORT_IF_INLINED
	#error rofl-pipeline has been compiled with packet processing API functions inlined, but target does not support it(ROFL_PIPELINE_ABORT_IF_INLINED). Please recompile rofl-datapath without --with-pipeline-platform-funcs-inlined
#endif"])
	AC_MSG_RESULT(yes)
])
AS_IF([test "x$with_pipeline_platform_funcs_inlined" != xyes],[
	AC_MSG_RESULT(no)
])

#Pipeline thread IDs
AC_MSG_CHECKING(the maximum number of threads/cpus for ROFL-pipeline packet processing API)
AC_ARG_WITH([pipeline-max-tids], AS_HELP_STRING([--with-pipeline-max-tids=num], [maximum number of threads/cpus that ROFL-pipeline packet processing API supports concurrently without locking [default=16]]), with_pipeline_max_tids=yes, [])

#Default value
MAX_TIDS=16
if test "$with_pipeline_max_tids" = "yes"; then
	MAX_TIDS=$withval
fi
AC_MSG_RESULT($MAX_TIDS)

AC_SUBST([ROFL_PIPELINE_MAX_TIDS], ["#define ROFL_PIPELINE_MAX_TIDS $MAX_TIDS"])
AC_SUBST([ROFL_PIPELINE_LOCKED_TID], ["#define ROFL_PIPELINE_LOCKED_TID 0"])

#Pipeline lockless
AC_ARG_WITH([pipeline-lockless], AS_HELP_STRING([--with-pipeline-lockless], [compiles ROFL-pipeline packet processing API without locking [default=no]]))
AC_MSG_CHECKING(whether to compile ROFL-pipeline packet processing API without locking)
AS_IF([test "x$with_pipeline_lockless" == xyes],[
	AC_SUBST([ROFL_PIPELINE_LOCKLESS], ["#define ROFL_PIPELINE_LOCKLESS 1"])
	AC_MSG_RESULT(yes)
])
AS_IF([test "x$with_pipeline_lockless" != xyes], [
	AC_MSG_RESULT(no)
])

#Cache aligned

##TODO deduce cache line from /proc/sys
AC_MSG_CHECKING(CPU cache line)
AC_MSG_RESULT([64 (TODO)])
AC_SUBST([ROFL_PIPELINE_CACHE_LINE], ["#define ROFL_PIPELINE_CACHE_LINE 64"] ) #TODO

AC_ARG_WITH([pipeline-cache-aligned], AS_HELP_STRING([--with-pipeline-cache-aligned], [compiles ROFL-pipeline with cache-aligned structures [default=no]]))
AC_MSG_CHECKING(whether to compile ROFL-pipeline structures cache aligned)
AS_IF([test "x$with_pipeline_cache_aligned" == xyes],[
	AC_SUBST([ROFL_PIPELINE_CACHE_ALIGNED], ["#define ROFL_PIPELINE_CACHE_ALIGNED 1"])
	AC_MSG_RESULT(yes)
])
AS_IF([test "x$with_pipeline_cache_aligned" != xyes], [
	AC_MSG_RESULT(no)
])
