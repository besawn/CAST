#!/bin/bash

#================================================================================
#   
#    hcdiag/src/tests/hxeewm/hxeewm.sh
# 
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#=============================================================================

## These lines are mandatory, so the framework knows the name of the log file
## This is necessary when invoked standalone --with xcat-- and common_fs=yes
if [ -n "$HCDIAG_LOGDIR" ]; then
   THIS_LOG=$HCDIAG_LOGDIR/`hostname -s`-`date +%Y-%m-%d-%H_%M_%S`.output
   echo $THIS_LOG
   exec 2>$THIS_LOG 1>&2
fi
[ $# -lt 1 ] && echo "Usage: $me <duration> [debug]" && exit 1

readonly me=${0##*/}
thisdir=`dirname $0`

source $thisdir/../common/htx_functions
source $thisdir/../common/functions
supported_machine
if [ "$ret" -ne "0" ]; then echo "$me test FAIL, rc=$ret"; exit $ret; fi 
echo "Running $me on `hostname -s`, machine type $model."          

debug=0
duration=$1
if [ $# -gt 1 ]; then debug=$2; fi


# assumes p9 mdt
thismdt="${htxd_root}/mdt/mdt.tdp"

# validates the enviroment to run htx and starts the htxd daemon
htxdiag_setup $debug
rc=$ret

if [ $rc -eq 0 ]; then 
   # creates the mdt file to run hxeewm
   htxdiag_get_mdt "None" $thismdt 
   rc=$ret
   if [ $rc -eq 0 ]; then 
      # run the test and validates
      htxdiag_run $thismdt $duration $debug
      rc=$ret
   fi

   # remove the files and stop the daemon
   htxdiag_cleanup
fi

if [ $rc -eq 0 ]; then
  echo "$me test PASS, rc=0"
else
  echo "$me test FAIL, rc=$rc"
fi

exit $rc
