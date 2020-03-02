
## Dependencies

Apollo requires the following dependencies to build:
* SOSflow
* Python 3.7+ (w/CFFI module)

Apollo is currently built and tested with **GCC 4.9.3** or **Intel/19**
in a **TOSS3** environment on **CZQuartz**. Make sure to load
or swap in the correct compiler module before initializing this build process.

Most of the scripts are preconfigured to look for projects in $HOME/src

## Basic Installation Process

```bash
######
#
#  For a NEW installation:
#    Execute this sequence of commands from the location where you want to
#    base your installation of Apollo, eg. $HOME/src
#
#####
#
#
# NOTE: EVPath is no longer required by SOS.  If you wish you run
#       the version of SOS that uses EVPath, see the EVPath install
#       instructinos at the bottom of this document, and install
#       it before installing SOS.
#
#
#  -- SOS
#
cd $HOME/src
git clone https://github.com/cdwdirect/sos_flow.git
cd sos_flow
./go.hard
#
#  -- Callpath
#
cd $HOME/src
git clone https://github.com/LLNL/callpath.git
cd callpath
rm -rf ./build/$SYS_TYPE
mkdir ./build/$SYS_TYPE && cd ./build/$SYS_TYPE
cmake
    -D CMAKE_INSTALL_PREFIX=$HOME/src/callpath/install \
    -D CALLPATH_WALKER=backtrace  \
    -D CMAKE_BUILD_TYPE=Release \
    ../..
make -j && make -j install
#
#  -- Apollo
#
mkdir -p $HOME/src
cd $HOME/src
git clone https://github.com/llnl/apollo.git apollo
cd apollo
./go.hard
#
#  You made it!
#
#  Now, take a look at $APOLLO_DIR/jobs/job.blank.sh for a template
#  that sets up the full environment, LD_LIBRARY_PATH and PYTHONPATH,
#  experiment (Cleverleaf) binaries, etc.  These are tested to work on CZQuartz
#  with TOSS3.
#
#  -- At this point, you are DONE and ready to go build examples!



#  --------------
#
##  OPTIONAL: EVPath installation steps (before SOS, if using EVPath variant):
##
##  -- EVPath  (https://www.cc.gatech.edu/systems/projects/EVPath/)
##
##cd $HOME/src
##mkdir evpath
##cd evpath
##mkdir install
##wget http://www.cc.gatech.edu/systems/projects/EVPath/chaos_bootstrap.pl
##perl ./chaos_bootstrap.pl stable `pwd`/install
##perl ./chaos_build.pl

```
