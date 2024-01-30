// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_TIMER_H
#define APOLLO_TIMER_H

#include <memory>

#include "apollo/Apollo.h"

class Apollo::Timer
{
public:
  Timer() {}
  virtual ~Timer() {}

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isDone(double &metric) = 0;

  template <typename T>
  static std::unique_ptr<Timer> create();

  struct Sync;
  struct CudaAsync;
  struct HipAsync;
};  // end: Timer (abstract class)


#endif