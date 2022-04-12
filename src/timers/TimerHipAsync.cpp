// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "timers/TimerHipAsync.h"

#include <iostream>

#define hipErrchk(ans)                    \
  {                                       \
    hipAssert((ans), __FILE__, __LINE__); \
  }

static inline void hipAssert(hipError_t code, const char *file, int line)
{
  if (code != hipSuccess) {
    std::cerr << "HIP error " << hipGetErrorString(code) << " at " << file
              << ":" << line << std::endl;
    abort();
  }
}

template <>
std::unique_ptr<Apollo::Timer> Apollo::Timer::create<Apollo::Timer::HipAsync>()
{
  return std::make_unique<TimerHipAsync>();
}

TimerHipAsync::TimerHipAsync()
{
  hipErrchk(hipEventCreateWithFlags(&event_start, hipEventDefault));
  hipErrchk(hipEventCreateWithFlags(&event_stop, hipEventDefault));
}

TimerHipAsync::~TimerHipAsync()
{
  hipErrchk(hipEventDestroy(event_start));
  hipErrchk(hipEventDestroy(event_stop));
}

void TimerHipAsync::start()
{
  // TODO: handle multiple streams.
  hipErrchk(hipEventRecord(event_start, 0));
}

void TimerHipAsync::stop() { hipErrchk(hipEventRecord(event_stop, 0)); }

bool TimerHipAsync::isDone(double &metric)
{
  float hipTime;

  static hipError_t code;
  if ((code = hipEventElapsedTime(&hipTime, event_start, event_stop)) !=
      hipSuccess) {
    if (code == hipErrorNotReady) return false;

    hipAssert(code, __FILE__, __LINE__);
  }

  // Convert from ms to s
  metric = hipTime / 1000.0;

  return true;
}
