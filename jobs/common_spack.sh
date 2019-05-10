#!/bin/bash
echo "module swap intel/18.0.1 gcc/4.9.3"
module swap intel/18.0.1 gcc/4.9.3
echo ""
echo "Compiler path: '$(which gcc)'"
echo "Compiler version:"
gcc --version
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
echo " |    |-- python"
spack load python
echo " |    |    |-- py-scipy"
spack load py-scipy
echo " |    |    |-- py-scikit-learn"
spack load py-scikit-learn
echo " |    |    |-- py-pandas"
spack load py-pandas
echo " |    |    |-- py-numpy"
spack load py-numpy
echo " |    |    |-- py-cffi"
spack load py-cffi
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

