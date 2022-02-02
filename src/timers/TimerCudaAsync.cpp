// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "timers/TimerCudaAsync.h"

#include <iostream>

#define cudaErrchk(ans)                    \
  {                                        \
    cudaAssert((ans), __FILE__, __LINE__); \
  }

static inline void cudaAssert(cudaError_t code, const char *file, int line)
{
  if (code != cudaSuccess) {
    std::cerr << "CUDA error " << cudaGetErrorString(code) << " at " << file
              << ":" << line << std::endl;
    abort();
  }
}

template <>
std::unique_ptr<Timer> Timer::create<Timer::CudaAsync>()
{
  return std::make_unique<TimerCudaAsync>();
}

TimerCudaAsync::TimerCudaAsync()
{
  cudaErrchk( cudaEventCreateWithFlags(&event_start, cudaEventDefault) );
  cudaErrchk( cudaEventCreateWithFlags(&event_stop, cudaEventDefault) );
}

TimerCudaAsync::~TimerCudaAsync()
{
  cudaErrchk( cudaEventDestroy(event_start) );
  cudaErrchk( cudaEventDestroy(event_stop) );
}

void TimerCudaAsync::start()
{
  // TODO: handle multiple streams.
  cudaErrchk( cudaEventRecord(event_start, 0) );
}

void TimerCudaAsync::stop()
{
  cudaErrchk( cudaEventRecord(event_stop, 0) );
}

bool TimerCudaAsync::isDone(double &metric)
{
  float cudaTime;

  static cudaError_t code;
  if ((code = cudaEventElapsedTime(&cudaTime, event_start, event_stop)) != cudaSuccess) {
    if (code == cudaErrorNotReady)
      return false;

    cudaAssert(code, __FILE__, __LINE__);
  }

  // Convert from ms to s
  metric = cudaTime / 1000.0;
  return true;
}
