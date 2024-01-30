// Copyright (c) 2015-2024, Lawrence Livermore National Security, LLC and other
// Apollo project developers. Produced at the Lawrence Livermore National
// Laboratory. See the top-level LICENSE file for details.
// SPDX-License-Identifier: MIT

#include "apollo/models/Static.h"

#include <cstring>
#include <string>

namespace apollo
{

int Static::getIndex(std::vector<float> &features) { return policy_choice; }

}  // end namespace apollo.