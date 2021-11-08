#include "OutputFormatter.h"

OutputFormatter::OutputFormatter(std::ostream &os)
    : os(os), level(0)
{
}

OutputFormatter &OutputFormatter::operator++() {
  level++;
  return *this;
}

OutputFormatter &OutputFormatter::operator--() {
  level--;
  return *this;
}