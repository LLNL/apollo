#include <cuda.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include "apollo/Apollo.h"
#include "apollo/Region.h"

__global__ void kernel_add(double* a, double* b, double c, int size)
{
  int id = blockIdx.x * blockDim.x + threadIdx.x;
  if (id < size) a[id] += b[id] * c;
}

void checkResult(double* v1, double* v2, int len);
void printResult(double* v, int len);

int main()
{

  std::cout << "\n\ndaxpy example...\n";

  //
  // Define vector length
  //
  const int N = 100000;
  // const int N = 1000;

  //
  // Allocate and initialize vector data.
  //
  double* a0 = new double[N];
  double* aref = new double[N];

  double* ta = new double[N];
  double* tb = new double[N];

  double c = 3.14159;

  for (int i = 0; i < N; i++) {
    a0[i] = 1.0;
    tb[i] = 2.0;
  }

  double *d_a, *d_b;
  cudaMalloc((void**)&d_a, sizeof(double) * N);
  cudaMalloc((void**)&d_b, sizeof(double) * N);

  //
  // Declare and set pointers to array data.
  // We reset them for each daxpy version so that
  // they all look the same.
  //

  double* a = ta;
  double* b = tb;


  //----------------------------------------------------------------------------//

  std::cout << "\n Running C-version of daxpy...\n";

  std::memcpy(a, a0, N * sizeof(double));

  for (int i = 0; i < N; ++i) {
    a[i] += b[i] * c;
  }

  std::memcpy(aref, a, N * sizeof(double));

  //----------------------------------------------------------------------------//

  //----------------------------------------------------------------------------//
  Apollo* apollo = Apollo::instance();
  Apollo::Region* r = new Apollo::Region(/*num_features*/ 1,
                                         "cuda-region",
                                         /*num_policies*/ 2);


  for (int j = 0; j < 20; j++) {

    std::memcpy(a, a0, N * sizeof(double));

    cudaMemcpy(d_a, a, sizeof(double) * N, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, sizeof(double) * N, cudaMemcpyHostToDevice);

    Apollo::RegionContext *ctx = r->begin({N});
    int policy = r->getPolicyIndex();
    std::cout << "EXECUTION " << j << "\n";
    // Create and install a CudaAsync Timer for this context to override the
    // default Sync Timer. The new timer must be started by the user. Apollo
    // will stop the timer at region->end() as before.
    ctx->timer = Timer::create<Timer::CudaAsync>();
    // Start the timer.
    ctx->timer->start();

    int block_size;
    switch(policy) {
      case 0:
        block_size = 1; break;
      case 1:
        block_size = 1024; break;
      default:
        abort();
    }

    int grid_size = std::ceil((float)N / block_size);
    kernel_add<<<grid_size, block_size>>>(d_a, d_b, c, N);

    cudaMemcpy(a, d_a, sizeof(double) * N, cudaMemcpyDeviceToHost);

    r->end();

    checkResult(a, aref, N);

    if(j==10)
        apollo->train(0);
  }


  //----------------------------------------------------------------------------//

  //----------------------------------------------------------------------------//

  //----------------------------------------------------------------------------//

  //
  // Clean up.
  //
  delete[] a0;
  delete[] aref;
  delete[] ta;
  delete[] tb;

  std::cout << "\n DONE!...\n";

  return 0;
}

//
// Function to compare result to reference and report P/F.
//
void checkResult(double* v1, double* v2, int len)
{
  bool match = true;
  for (int i = 0; i < len; i++) {
    if (v1[i] != v2[i]) {
      match = false;
    }
  }
  if (match) {
    std::cout << "\n\t result -- PASS\n";
  } else {
    std::cout << "\n\t result -- FAIL\n";
  }
}

//
// Function to print result.
//
void printResult(double* v, int len)
{
  std::cout << std::endl;
  for (int i = 0; i < len; i++) {
    std::cout << "result[" << i << "] = " << v[i] << std::endl;
  }
  std::cout << std::endl;
}
