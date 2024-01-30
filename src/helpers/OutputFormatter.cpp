// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "OutputFormatter.h"

OutputFormatter::OutputFormatter(std::ostream &os) : os(os), level(0) {}

OutputFormatter &OutputFormatter::operator++()
{
  level++;
  return *this;
}

OutputFormatter &OutputFormatter::operator--()
{
  level--;
  return *this;
}