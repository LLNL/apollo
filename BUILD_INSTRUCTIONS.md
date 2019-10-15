
## Dependencies

Apollo requires the following dependencies to build:
* SOSflow
* EVPath
* Python 2.7 w/CFFI enabled
* Callpath
* RAJA
* Cleverleaf (for custom RAJA Apollo policy and default example code)

Apollo is currently built and tested with **GCC 4.9.3** in a **TOSS3**
environment on **CZQuartz**. Make sure to load
or swap in the correct compiler module before initializing this build process:

`module swap -q intel/18.0.1 gcc/4.9.3`

More recent compilers may work, but can get caught on warnings or
impose stricter rules which may trip you up.

## Basic Installation Process

Until Apollo and its dependencies are fully Spack-ified, the following
sequence of commands should get you in the ballpark.
```bash
######
#
#  For a NEW installation:
#    Execute this sequence of commands from the location where you want to
#    base your installation of Apollo, eg. $HOME/src
#
#####
#
#  -- Apollo (1/2: Clone source code)
#
mkdir -p $HOME/src
cd $HOME/src
git clone https://github.com/llnl/apollo.git apollo
cd apollo
export APOLLO_DIR=`pwd`
#
#  -- EVPath  (https://www.cc.gatech.edu/systems/projects/EVPath/)
#
cd $HOME/src
mkdir evpath
cd evpath
mkdir install
wget http://www.cc.gatech.edu/systems/projects/EVPath/chaos_bootstrap.pl
perl ./chaos_bootstrap.pl stable `pwd`/install
perl ./chaos_build.pl
#
#  -- SOS
#
cd $HOME/src
git clone https://github.com/cdwdirect/sos_flow.git
cd sos_flow
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=`pwd` -DEVPath_DIR=$HOME/src/evpath/install ..
make -j && make -j install
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
#  -- Cleverleaf (1/2: Clone source, build RAJA for Apollo)
#
# NOTE: Cleverleaf build (and Apollo's job launch scripts) expect it to be
#       installed in $HOME/src/cleverleaf ... if you're tucking
#       Cleverleaf into another path, such as $APOLLO_DIR/depends, you
#       will need to edit more than just the Cleverleaf build scripts.
#
# NOTE: These "go" and "configure" scripts are subject to
#       developers breaking or hardcoding pathing and applying
#       customized linking instructions during R&D cycles. Expect
#       a little fritction at the Cleverleaf steps, and set aside some
#       time to manually review the details of the Cleverleaf
#       build scripts.  If you make your own versions, feel free to add
#       them to .gitignore
#
cd $HOME/src
git clone https://github.com/cdwdirect/cleverleaf
./go.raja apollo RelWithDebInfo && ./go.raja apollo Release
./go.raja normal RelWithDebInfo && ./go.raja normal Release
#
#  -- Apollo (2/2: Build Apollo w/Cleverleaf's RAJA)
#
cd $APOLLO_DIR
./go.hard
#
#  -- Cleverleaf (2/2: Build full Cleverleaf w/Apollo)
#
cd $HOME/src/cleverleaf
cd cleverleaf
./go.hard apollo RelWithDebInfo && ./go.hard apollo Release
./go.hard normal RelWithDebInfo && ./go.hard normal Release
#
#  You made it!
#
#  Now, take a look at $APOLLO_DIR/jobs/job.blank.sh for a template
#  that sets up the full environment, LD_LIBRARY_PATH and PYTHONPATH,
#  Cleverleaf binaries, etc.  These are tested to work on CZQuartz
#  with TOSS3.
#
```
