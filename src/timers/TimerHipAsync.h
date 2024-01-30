// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_TIMER_HIP_ASYNC_H
#define APOLLO_TIMER_HIP_ASYNC_H

#include <hip/hip_runtime.h>

#include "apollo/Timer.h"

class TimerHipAsync : public Apollo::Timer
{
public:
  TimerHipAsync();
  ~TimerHipAsync();
  void start();
  void stop();
  bool isDone(double &metric);

private:
  hipEvent_t event_start;
  hipEvent_t event_stop;
};

#endif