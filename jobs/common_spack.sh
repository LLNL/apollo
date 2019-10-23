#!/bin/bash
#echo "module swap gcc/4.9.3 intel/19.0.4 "
#module swap gcc/4.9.3 intel/19.0.4
echo "module load intel/19.0.4"
module load intel/19.0.4
if [ "x${CC}" == "x" ] ; then
    export CC=$(which icc)
fi
if [ "x${CXX}" == "x" ] ; then
    export CXX=$(which icpc)
fi
if [ "x${FC}" == "x" ] ; then
    export FC=$(which ifort)
fi
if [ "x${F90}" == "x" ] ; then
    export F90=$(which ifort)
fi
export MPICXX=mpicxx
export MPI_CXX=mpicxx

echo ""
# Absolute path this script is in:
export SETENV_SCRIPT_PATH="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"

if [ "x$SOS_ENV_SET" == "x1" ] ; then
	echo "WARNING: You SOS environment is already configured."
    echo "         Please run the unsetenv.sh script first:"
    echo ""
    echo "    $SETENV_SCRIPT_PATH/unsetenv.sh"
    echo ""
    kill -INT $$
fi
echo ""
echo " .-- Processing ~/setenv.sh ..."
echo " |"
echo " |-- Initializing Spack environment..."
export SPACK_ROOT='/g/g17/wood67/src/spack'
export PATH=$SPACK_ROOT/bin:$PATH
. $SPACK_ROOT/share/spack/setup-env.sh
echo " |-- Loading Spack packages..."
export PY_SPACK_HASH="zsgpqf3"    #Python 2.7.16
#export PY_SPACK_HASH="tckxnhk"    #Python 3.7.2
echo " |    |-- python@2.7.16                      /$PY_SPACK_HASH"
spack load python@2.7.16
echo " |    |    |-- py-scipy"
spack load py-scipy ^python/$PY_SPACK_HASH
echo " |    |    |-- py-scikit-learn"
spack load py-scikit-learn ^python/$PY_SPACK_HASH
echo " |    |    |-- py-pandas"
spack load py-pandas ^python/$PY_SPACK_HASH
echo " |    |    |-- py-numpy"
spack load py-numpy ^python/$PY_SPACK_HASH
echo " |    |    |-- py-pycparser"
spack load py-pycparser ^python/$PY_SPACK_HASH
echo " |    |    |-- py-cffi"
spack load py-cffi ^python/$PY_SPACK_HASH
echo " |    |    |-- py-pytz"
spack load py-pytz ^python/$PY_SPACK_HASH
echo " |    |    |-- py-dateutil"
spack load py-dateutil ^python/$PY_SPACK_HASH
echo " |    |    |-- py-six"
spack load py-six ^python/$PY_SPACK_HASH
echo " |    |    \\_"
echo " |    \\_"
echo " |-- vim"
spack load vim
echo " |-- cmake"
spack load cmake
echo " |-- libnl"
spack load libnl
echo " |-- libevpath"
spack load libevpath
echo " |-- libffs"
spack load libffs
echo " |-- gtkorvo-atl"
spack load gtkorvo-atl
echo " |-- gtkorvo-dill"
spack load gtkorvo-dill
echo " |-- htop"
spack load htop
echo " \\_"
echo ""

#  NOTE: If we decide to set these paths for experiments...
#        ...it makes unsetting the environment a bit trickier.
#
#echo "Updating paths:"
#echo "     \$PATH            += \$SOS_BASE/scripts"
#export PATH="${PATH}:$SOS_BASE/scripts"
#
#echo "     \$PYTHONPATH      += \$SOS_BUILD_DIR/lib"
#if [ "x$PYTHONPATH" == "x" ] ; then
#  PYTHONPATH=$SOS_BASE/src/python
#else
#  PYTHONPATH=$SOS_BASE/src/python:$PYTHONPATH
#fi
#
#echo "     \$LD_LIBRARY_PATH += \$SOS_BUILD_DIR/lib"
#if [ "x$LD_LIBRARY_PATH" == "x" ] ; then
#  LD_LIBRARY_PATH=$SOS_BUILD_DIR/lib
#else
#  LD_LIBRARY_PATH=$SOS_BUILD_DIR/lib:$LD_LIBRARY_PATH
#fi
#

