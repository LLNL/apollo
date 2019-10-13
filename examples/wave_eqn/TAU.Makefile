
TAU_MAKEFILE ?=/home/users/khuck/src/tau2/x86_64/lib/Makefile.tau-papi-pthread
include $(TAU_MAKEFILE)

CXX=mpicxx
LD=$(CXX)
PRINT=pr
RM=/bin/rm -rf

raja=/home/users/khuck/src/apollo/cleverleaf/package-apollo/Release/install/RAJA
apollo=/home/users/khuck/src/apollo_new/install
callpath=/home/users/khuck/src/apollo/spack/opt/spack/linux-centos7-skylake_avx512/gcc-8.1.0/callpath-1.0.4-psvqvudiangygnnsotdlidvincvkv53k
utils=/home/users/khuck/src/apollo/spack/opt/spack/linux-centos7-skylake_avx512/gcc-8.1.0/adept-utils-1.0.1-keloyanvpyzkdrm5fniwdtzorv7u3mxw
caliper=/home/users/khuck/src/apollo/Caliper/install
llvm=/home/users/khuck/src/openmp/final/install-gcc-Release

LIBS=-rdynamic ${raja}/lib/libRAJA.a -lstdc++ -lm -lgcc_s -lgcc -lc -lgcc_s -lgcc -L${apollo}/lib -lapollo -Wl,-rpath,${apollo}/lib -L${caliper}/lib64 -lcaliper -Wl,-rpath,${caliper}/lib64 -L${callpath}/lib -lcallpath -Wl,-rpath,${callpath}/lib -L${utils}/lib -ladept_utils -Wl,-rpath,${utils}/lib $(TAU_LIBS)
CFLAGS=-g -O3 -fopenmp -DNDEBUG -Wall -Wextra -std=c++1y -I${raja}/include -I${apollo}/include -I/${callpath}/include -I${utils}/include $(TAU_INCLUDE) $(TAU_DEFS)
#LDFLAGS=-g -O3 -fopenmp
LDFLAGS=-g -O3 -L${llvm}/lib -Wl,-rpath,${llvm}/lib -lomp

##############################################

all:	wave-eqn-apollo wave-eqn-normal

wave-eqn-apollo:	wave-eqn-apollo.o
	$(LD) $(LDFLAGS) $< -o $@ $(LIBS)

wave-eqn-apollo.o: wave-eqn.cpp Makefile
	$(CXX) $(CFLAGS) -c $< -o $@ -DRAJA_ENABLE_APOLLO

wave-eqn-normal:	wave-eqn-normal.o
	$(LD) $(LDFLAGS) $< -o $@ $(LIBS)

wave-eqn-normal.o: wave-eqn.cpp Makefile
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	$(RM) wave-eqn-normal.o wave-eqn-normal wave-eqn-apollo.o wave-eqn-apollo profile.* *.trc *.edf *.z MULT* *.inst.* *.pdb Comp_gnu.o *.pomp.c *.opari.inc pompregions.* *.output *.error *.cobaltlog *.ppk traces*
##############################################
