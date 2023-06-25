#ifndef APOLLO_ERROR_HANDLING_H
#define APOLLO_ERROR_HANDLING_H

#include <iostream>

namespace apollo
{

static void fatal_error_internal(const std::string &msg,
                                 const char *file,
                                 const unsigned line)
{
  std::cerr << file << ":" << line << " " << msg << "\n";
  abort();
}

}  // namespace apollo

#define fatal_error(msg) apollo::fatal_error_internal(msg, __FILE__, __LINE__)

#endif