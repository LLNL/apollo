// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_TIMER_CUDA_ASYNC_H
#define APOLLO_TIMER_CUDA_ASYNC_H

#include <cuda_runtime.h>

#include "apollo/Timer.h"

class TimerCudaAsync : public Timer
{
public:
  TimerCudaAsync();
  ~TimerCudaAsync();
  void start();
  void stop();
  bool isDone(double &metric);

private:
  cudaEvent_t event_start;
  cudaEvent_t event_stop;
};

#endif