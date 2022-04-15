#include "helpers/Parser.h"

#include <iostream>

bool Parser::eof()
{
  pos = is.tellg();
  std::string tmp;
  is >> tmp;
  if (is.eof()) return true;
  is.seekg(pos);
  return false;
}

std::string Parser::getNextToken()
{
  pos = is.tellg();
  auto state = is.rdstate();
  is >> token;
  // std::cout << "parser token " << token << "\n";
  while (token[0] == '#') {
    is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    pos = is.tellg();
    is >> token;
  }

  next_pos = is.tellg();
  // is.setstate(state);
  is.clear(state);
  is.seekg(pos);

  return token;
}

void Parser::consumeToken() { is.seekg(next_pos); }

void Parser::error(const std::string &msg)
{
  is.clear();
  int err_pos = next_pos;
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
