#================================================================================
#
#    hcdiag/src/tests/chk-sys-firmware/CMakeLists.txt
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
#================================================================================

set(HCDIAG_TEST chk-sys-firmware)

file(GLOB HCDIAG_TEST_PROGRAM
  "chk-sys-firmware.sh"
  "chk-sys-firmware.pm"
)


install(PROGRAMS ${HCDIAG_TEST_PROGRAM} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/tests/${HCDIAG_TEST})

