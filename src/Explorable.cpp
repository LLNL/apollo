
// Copyright (c) 2019, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory
//
// This file is part of Apollo.
// OCEC-17-092
// All rights reserved.
//
// Apollo is currently developed by Chad Wood, wood67@llnl.gov, with the help
// of many collaborators.
//
// Apollo was originally created by David Beckingsale, david@llnl.gov
//
// For details, see https://github.com/LLNL/apollo.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <vector>
#include "assert.h"

#include "apollo/Apollo.h"
#include "apollo/Logging.h"
#include "apollo/Explorable.h"


    // NOTE: If we want to grab a GUID for some future annotation, this is how:
    // ----
    //SOS_guid guid = 0;
    //if (apollo->isOnline()) {
    //    guid = SOS_uid_next(sos->uid.my_guid_pool);
    //}

Apollo::Explorable::Explorable(
        std::string          set_name,
        double              *set_target_var,
        std::vector<int>     set_for_policies,
        std::vector<double>  set_values)
{
    assert (set_target_var != NULL);
    assert (set_target_var != nullptr);
    assert (set_for_policies.size() > 0);
    assert (set_values.size() > 0);

    name         = set_name;
    target_var   = set_target_var;

    offset = 0;
    *target_var  = values[0];

    return;
}



