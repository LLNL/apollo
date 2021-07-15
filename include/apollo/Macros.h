// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (MIT)

#ifndef APOLLO_MACROS_H
#define APOLLO_MACROS_H

#define APOLLO_FEATURE(apollo_ptr, name, goal, unit, var_ptr) \
    Apollo::Feature __apollo_feat##name

#endif
