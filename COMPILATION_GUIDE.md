
## Dependencies

Apollo has the following build dependencies:
 * [Callpath](https://github.com/llnl/callpath)
 * [OpenCV](https://github.com/opencv/opencv)

Notes:
 * MPI support is optional, and typically provided by the system.
 * Most of the scripts are preconfigured to look for projects in $HOME/src
 * Check out the paths in the CMake + Make script:  `go.hard`


## Basic Installation Process

```bash
######
#
#  For a NEW installation:
#    Execute this sequence of commands from the location where you want to
#    base your installation of Apollo, eg. $HOME/src
#
######
#
#  -- Callpath
#
cd $HOME/src
git clone https://github.com/LLNL/callpath.git
cd callpath
rm -rf ./install
rm -rf ./build/$SYS_TYPE
mkdir ./install
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
#  that sets up the full environment, LD_LIBRARY_PATH, optional PYTHONPATH,
#  experiment (Cleverleaf) binaries, etc.  These are tested to work on CZQuartz
#  with TOSS3.
#
#  -- At this point, you are DONE and ready to go build examples!
#
#####           #############################
#####   Done!   #############################
#####           #############################

```



```bash
#####
#
#  (Notes RE: Older experimental build w/SOS and Python+SKL)
#
Experimental async Apollo requires the following dependencies to build:
* SOSflow
* Python 3.7+ (w/CFFI module)

Apollo is currently built and tested with **GCC 4.9.3** or **Intel/19**
in a **TOSS3** environment on **CZQuartz**. Make sure to load
or swap in the correct compiler module before initializing this build process.

#
#  -- SOS
#
cd $HOME/src
git clone https://github.com/cdwdirect/sos_flow.git
cd sos_flow
./go.hard
#

-->
```
