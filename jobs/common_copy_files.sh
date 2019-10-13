#!/bin/bash
#
# Copy the binary, configuration, and plotting scripts into the folder
# where the output of the job is being stored.
#
mkdir -p ${SOS_WORK}
mkdir -p ${SOS_WORK}/output
mkdir -p ${SOS_WORK}/output/models
mkdir -p ${SOS_WORK}/launch
mkdir -p ${SOS_WORK}/daemons
mkdir -p ${SOS_WORK}/lib
mkdir -p ${SOS_WORK}/bin
mkdir -p ${SOS_WORK}/bin/apollo
#
cp ${HOME}/src/sos_flow/build/bin/sosd                            ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_stop                       ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_probe                      ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/sosd_manifest                   ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/build/bin/demo_app                        ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/scripts/showdb                            ${SOS_WORK}/bin
cp ${HOME}/src/sos_flow/src/python/ssos.py                        ${SOS_WORK}/bin
#
cp ${HOME}/src/apollo/src/python/controller.py                    ${SOS_WORK}/bin
cp ${HOME}/src/apollo/src/python/apollo/*                         ${SOS_WORK}/bin/apollo
#
cp ${HOME}/src/apollo/jobs/model.*                                ${SOS_WORK}
#
cp ${HOME}/src/apollo/src/python/SQL.CREATE.viewApollo            ${SOS_WORK}
cp ${HOME}/src/apollo/src/python/SQL.CREATE.indexApollo           ${SOS_WORK}
cp ${HOME}/src/apollo/src/python/SQL.sanityCheck                  ${SOS_WORK}
#
cp ${HOME}/src/apollo/install/lib/libapollo.so                    ${SOS_WORK}/lib
cp ${HOME}/src/sos_flow/build/lib/libsos.so                       ${SOS_WORK}/lib
cp ${HOME}/src/sos_flow/build/lib/ssos_python.so                  ${SOS_WORK}/lib
cp ${HOME}/src/callpath/install/lib/libcallpath.so                ${SOS_WORK}/lib
#
#cp ${HOME}/src/caliper/install/lib64/libcaliper.so                ${SOS_WORK}/lib
#
export PYTHONPATH=${SOS_WORK}/lib:${SOS_WORK}/bin:${PYTHONPATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${SOS_WORK}/lib
#
# Make an archive of this script and the environment config script:
#
cp ${RETURN_PATH}/common_spack.sh           ${SOS_WORK}/launch
cp ${RETURN_PATH}/common_setenv.sh          ${SOS_WORK}/launch
cp ${RETURN_PATH}/common_unsetenv.sh        ${SOS_WORK}/launch
cp ${RETURN_PATH}/common_copy_files.sh      ${SOS_WORK}/launch
cp ${RETURN_PATH}/sample.tau.conf           ${SOS_WORK}/launch
cp ${RETURN_PATH}/sample.tau.srun           ${SOS_WORK}/launch
#
#  Copy the applications into the experiment path:
#
cp ${HOME}/src/cleverleaf/package-apollo/Release/install/cleverleaf/bin/cleverleaf \
    ${SOS_WORK}/bin/cleverleaf-apollo-release
cp ${HOME}/src/cleverleaf/package-apollo/RelWithDebInfo/install/cleverleaf/bin/cleverleaf \
    ${SOS_WORK}/bin/cleverleaf-apollo-relwithdebinfo
cp ${HOME}/src/cleverleaf/package-normal/Release/install/cleverleaf/bin/cleverleaf \
    ${SOS_WORK}/bin/cleverleaf-normal-release
cp ${HOME}/src/cleverleaf/package-normal/RelWithDebInfo/install/cleverleaf/bin/cleverleaf \
    ${SOS_WORK}/bin/cleverleaf-normal-relwithdebinfo


