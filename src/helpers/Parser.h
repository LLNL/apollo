#ifndef APOLLO_HELPERS_PARSER_H
#define APOLLO_HELPERS_PARSER_H

#include <iostream>
#include <sstream>

class Parser
{
public:
  Parser(std::istream &is) : is(is){};

  template <typename T>
  void parse(T &out);

  void parseExpected(const char *expected);

  const char *getNextToken();
  const char *getToken() const;
  bool getTokenEquals(const char *s);
  bool getNextTokenEquals(const char *s);

private:
  std::istream &is;
  static constexpr size_t BUFSIZE = 64;
  char buffer[BUFSIZE];
  char *token;
  void error(const std::string &msg);
};


#endif
