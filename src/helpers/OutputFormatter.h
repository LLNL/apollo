#ifndef APOLLO_MODELS_OUTPUTFORMATTER_H
#define APOLLO_MODELS_OUTPUTFORMATTER_H

#include <iostream>

// Output formatting that indents by level (2 spaces) using overloaded operator
// << , or outputs inline, using overloaded operator &. Level is inc/dec by
// overloaded operators ++/--.
class OutputFormatter
{
public:
  OutputFormatter(std::ostream &os);

  template <typename T>
  OutputFormatter &operator<<(T output);

  template <typename T>
  OutputFormatter &operator&(T output);

  OutputFormatter &operator++();
  OutputFormatter &operator--();


private:
  std::ostream &os;
  unsigned level;
};

template <typename T>
OutputFormatter &OutputFormatter::operator<<(T output)
{
  for (unsigned int i = 0; i < level; ++i)
    os << "  ";
  os << output;
  return *this;
}

template <typename T>
OutputFormatter &OutputFormatter::operator&(T output)
{
  os << output;
  return *this;
}

#endif