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

  void parseExpected(const std::string &expected);

  const std::string &getNextToken();
  const std::string &getToken();

  bool eof();

private:
  std::istream &is;
  std::stringstream ss;
  std::string token;
  void error(const std::string &msg);
};

template <typename T>
void Parser::parse(T &val)
{
  // std::cout << "Parsing " << typeid(T).name() << "\n";
  if (!(ss >> val)) error("Not matching type " + std::string(typeid(T).name()));
}

#endif
