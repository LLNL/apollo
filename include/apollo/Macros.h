// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#ifndef APOLLO_MACROS_H
#define APOLLO_MACROS_H

#define APOLLO_FEATURE(apollo_ptr, name, goal, unit, var_ptr) \
    Apollo::Feature __apollo_feat##name

#endif
