/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA apollo policy definitions.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-18, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For details about use and distribution, please read RAJA/LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef policy_apollo_HPP
#define policy_apollo_HPP

#include "RAJA/policy/PolicyBase.hpp"

namespace RAJA
{
namespace policy
{
namespace apollo
{

//
//////////////////////////////////////////////////////////////////////
//
// Execution policies
//
//////////////////////////////////////////////////////////////////////
//

///
/// Segment execution policies
///

struct apollo_region : make_policy_pattern_launch_platform_t<Policy::apollo,
                                                          Pattern::region,
                                                          Launch::sync,
                                                          Platform::host> {
};

struct apollo_exec : make_policy_pattern_launch_platform_t<Policy::apollo,
                                                        Pattern::forall,
                                                        Launch::undefined,
                                                        Platform::host> {
};

///
/// Index set segment iteration policies
///
using apollo_segit = apollo_exec;

///
///////////////////////////////////////////////////////////////////////
///
/// Reduction execution policies
///
///////////////////////////////////////////////////////////////////////
///
struct apollo_reduce : make_policy_pattern_launch_platform_t<Policy::apollo,
                                                          Pattern::forall,
                                                          Launch::undefined,
                                                          Platform::host> {
};

}  // end namespace apollo 
}  // end namespace policy

using policy::apollo::apollo_exec;
using policy::apollo::apollo_region;
using policy::apollo::apollo_segit;
using policy::apollo::apollo_reduce;


///
///////////////////////////////////////////////////////////////////////
///
/// Shared memory policies
///
///////////////////////////////////////////////////////////////////////
///

struct apollo_shmem {
};

}  // closing brace for RAJA namespace

#endif
