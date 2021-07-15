// Copyright 2017-2021 Lawrence Livermore National Security, LLC and other
// Apollo Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (MIT)

#ifndef APOLLO_UTIL_COLOR
#define APOLLO_UTIL_COLOR

#define RST   "\x1B[0m"
#define KBOLD "\x1B[1m"
#define KUNDL "\x1B[4m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define FRED(x) KRED x RST
#define FGRN(x) KGRN x RST
#define FYEL(x) KYEL x RST
#define FBLU(x) KBLU x RST
#define FMAG(x) KMAG x RST
#define FCYN(x) KCYN x RST
#define FWHT(x) KWHT x RST

#define BOLD(x) "\x1B[1m" x RST
#define UNDL(x) "\x1B[4m" x RST


#define sRST    std::string(RST)
#define sBOLD   std::string(KBOLD)
#define sUNDL   std::string(KUNDL)
#define sRED    std::string(KRED)
#define sGRN    std::string(KGRN)
#define sYEL    std::string(KYEL)
#define sBLU    std::string(KBLU)
#define sMAG    std::string(KMAG)
#define sCYN    std::string(KCYN)
#define sWHT    std::string(KWHT)



#endif
