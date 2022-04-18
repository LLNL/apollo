// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_HELPERS_TIMETRACE_H
#define APOLLO_HELPERS_TIMETRACE_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "apollo/Timer.h"

// Simple time tracing class, outputs time elapsed in a block scope.
class TimeTrace
{
  std::string ref;
  std::unique_ptr<Apollo::Timer> timer;

public:
  TimeTrace(std::string ref)
      : ref(ref), timer(Apollo::Timer::create<Apollo::Timer::Sync>())
  {
    timer->start();
  }
  ~TimeTrace()
  {
    timer->stop();
    double duration;
    timer->isDone(duration);
    std::stringstream outs;
    outs << std::fixed << std::setprecision(0) << duration * 1e6;
    std::cout << "=== T ref " << ref << " = " << outs.str() << " us"
              << std::endl;
  }
};

#endif