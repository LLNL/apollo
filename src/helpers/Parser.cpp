#include "helpers/Parser.h"

#include <iostream>

bool Parser::eof()
{
  std::streampos pos = is.tellg();
  std::string tmp;
  is >> tmp;
  if (is.eof()) return true;
  is.seekg(pos);
  return false;
}

const std::string &Parser::getToken() { return token; }

const std::string &Parser::getNextToken()
{
  is >> token;
  // std::cout << "parser token " << token << "\n";
  while (token[0] == '#') {
    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    is >> token;
  }

  ss.clear();
  ss.str(token);

  return token;
}

void Parser::error(const std::string &msg)
{
  is.clear();
  int err_pos = is.tellg();
  is.seekg(0);
  int col = 1;
  int lineno = 1;
  // Find the line, line number, and column of the error.
  std::string line;
  while (std::getline(is, line)) {
    if (is.tellg() >= err_pos) break;
    lineno++;
    col = is.tellg();
  }

  col = err_pos - col;
  std::cerr << line << "\n";

  for (int j = 0; j < col - 1; ++j)
    std::cerr << " ";
  std::cerr << "^\n";
  std::cerr << "Line: " << lineno << " Col: " << col << ", Parse error: " << msg
            << "\n";
  abort();
}

void Parser::parseExpected(const std::string &expected)
{
  std::string out;
  ss >> out;
  if (out != expected) {
    std::stringstream error_msg;
    error_msg << "Expected \"" << expected << "\" but parsed \"" << out << "\"";
    error(error_msg.str());
  }
}
