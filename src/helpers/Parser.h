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

  template <typename T>
  void parseExpected(const T &expected);
  void parseExpectedString(const std::string &expected);

  std::string getNextToken();

  bool eof();

  void consumeToken();

private:
  std::istream &is;
  std::streampos pos, next_pos;
  std::string token;
  void error(const std::string &msg);
};

template <typename T>
void Parser::parse(T &val)
{
  // std::cout << "Parsing " << typeid(T).name() << "\n";
  if (!(is >> val)) error("Not matching type " + std::string(typeid(T).name()));
}

template <typename T>
void Parser::parseExpected(const T &expected)
{
  T out;
  is >> out;
  if (out != expected) {
    std::stringstream error_msg;
    error_msg << "Expected \"" << expected << "\" but parsed \"" << out << "\"";
    error(error_msg.str());
  }
}

#endif