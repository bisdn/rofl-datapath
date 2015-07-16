#ROFL pipeline

AC_SUBST([ROFL_PIPELINE_PRESENT], ["#define ROFL_PIPELINE_PRESENT 1"])

## Matching algorithms
MATCHING_ALGORITHMS_DIR="src/rofl/datapath/pipeline/openflow/openflow1x/pipeline/matching_algorithms"
AC_SUBST(MATCHING_ALGORITHMS_DIR)
MATCHING_ALGORITHMS="trie loop l2hash"
MATCHING_ALGORITHM_LIBS=""
MATCHING_ALGORITHM_LIBADD=""

# pipeline matching algorithms
AC_ARG_ENABLE(matching-algorithms,
	AS_HELP_STRING([--enable-matching-algorithms="list of matching algorithms"],
  		[Build support for the list of matching algorithms. The 
  		default is to build the loop matching algorithm.]),
[ case $enableval in
  yes)
  		# add all algorithms in $MATCHING_ALGORITHMS_DIR
        for dir in $srcdir/$MATCHING_ALGORITHMS_DIR/*; do
            algorithm="`basename $dir`"
            if test -d "$dir"; then
                MATCHING_ALGORITHMS="$MATCHING_ALGORITHMS $algorithm"
            fi
        done
        ;;
  no)
        ;;
  *)
        MATCHING_ALGORITHMS="`echo $enableval| sed -e 's/,/ /g;s/  */ /g'`"
        ;;
  esac
],[])
if test -n "$MATCHING_ALGORITHMS"; then
    # ensure that all algorithms a) exist and b) only include once.
    MATCHING_ALGORITHMS_FULL=$MATCHING_ALGORITHMS
    MATCHING_ALGORITHMS=
    for algorithm in $MATCHING_ALGORITHMS_FULL; do
        have_alg=`echo "$MATCHING_ALGORITHMS" | grep "$algorithm"`
        if test "$have_alg" != ""; then
            AC_MSG_NOTICE([Removing duplicate $algorithm from matching algorithms])
        elif test -d "$srcdir/$MATCHING_ALGORITHMS_DIR/$algorithm"; then
            MATCHING_ALGORITHMS="$MATCHING_ALGORITHMS $algorithm"
            MATCHING_ALGORITHM_LIBADD="$MATCHING_ALGORITHM_LIBADD librofl_pipeline_openflow1x_pipeline_matching_algorithms_$algorithm.la"
        else
            MATCHING_ALGORITHMS="$MATCHING_ALGORITHMS $algorithm"
        	MATCHING_ALGORITHM_AS_LIBS="$MATCHING_ALGORITHM_AS_LIBS $algorithm"
            MATCHING_ALGORITHM_LIBS="MATCHING_ALGORITHM_LIBS -lrofl_pipeline_openflow1x_pipeline_matching_algorithms_$algorithm"
        fi
    done
    AC_MSG_NOTICE([Matching algorithms built: $MATCHING_ALGORITHMS])
    AC_MSG_NOTICE([Matching algorithms as libs: $MATCHING_ALGORITHM_AS_LIBS])
fi

for algorithm in $MATCHING_ALGORITHMS; do
    HAVE_MATHING_ALGORITHM=HAVE_MA_`echo $algorithm | sed 's/\(.*\)/\U\1/'`
    AC_DEFINE_UNQUOTED([$HAVE_MATHING_ALGORITHM], [1], [Description])
done

AC_SUBST(MATCHING_ALGORITHM_LIBADD)
AC_SUBST(MATCHING_ALGORITHM_LIBS)
AC_SUBST(MATCHING_ALGORITHMS)
