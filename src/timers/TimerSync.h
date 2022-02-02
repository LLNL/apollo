// Copyright (c) 2015-2022, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#ifndef APOLLO_TIMER_SYNC_H
#define APOLLO_TIMER_SYNC_H

#include "apollo/Timer.h"

class TimerSync : public Timer
{
public:
  TimerSync(){};
  ~TimerSync(){};
  void start();
  void stop();
  bool isDone(double &metric);

private:
  double exec_time_begin, exec_time_end;
};

#endif