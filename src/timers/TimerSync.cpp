#include "timers/TimerSync.h"

#include <chrono>
#include <iostream>

template<>
std::unique_ptr<Timer> Timer::create<Timer::Sync>()
{
  return std::make_unique<TimerSync>();
}

void TimerSync::start()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  exec_time_begin = ts.tv_sec + ts.tv_nsec / 1e9;
}

void TimerSync::stop()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  exec_time_end = ts.tv_sec + ts.tv_nsec / 1e9;
}

bool TimerSync::isDone(double &metric)
{
  metric = exec_time_end - exec_time_begin;
  return true;
}
