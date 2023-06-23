#include "helpers/Parser.h"

#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>

#include "helpers/ErrorHandling.h"

const char *Parser::getToken() const { return token; }

bool Parser::getTokenEquals(const char *s)
{
  return (std::strcmp(token, s) == 0);
}

const char *Parser::getNextToken()
{
  std::istreambuf_iterator<char> it(is);
  std::istreambuf_iterator<char> end;

  int idx = 0;
  if (it != end) {
    while (std::isspace(*it))
      ++it;

    while (*it == '#') {
      is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      while (std::isspace(*it))
        ++it;
    }

    while (!std::isspace(*it)) {
      buffer[idx] = *it;
      ++idx;
      ++it;
      if (idx >= BUFSIZE) fatal_error("Token exceeds buffer size");
    }
  }
  buffer[idx] = '\0';

  token = buffer;

  return token;
}

bool Parser::getNextTokenEquals(const char *s)
{
  getNextToken();
  return getTokenEquals(s);
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

void Parser::parseExpected(const char *expected)
{
  if (std::strcmp(token, expected)) {
    std::stringstream error_msg;
    error_msg << "Expected \"" << expected << "\" but parsed \"" << token
              << "\"";
    error(error_msg.str());
  }
}

template <>
void Parser::parse(int &val)
{
  std::size_t pos;
  val = std::stoi(token, &pos);
  token = &buffer[pos];
}

template <>
void Parser::parse(double &val)
{
  std::size_t pos;
  val = std::stod(token, &pos);
  token = &buffer[pos];
}

template <>
void Parser::parse(float &val)
{
  std::size_t pos;
  val = std::stof(token, &pos);
  token = &buffer[pos];
}


template <>
void Parser::parse(unsigned int &val)
{
  std::size_t pos;
  val = std::stoul(token, &pos);
  token = &buffer[pos];
}


template <>
void Parser::parse(unsigned long &val)
{
  std::size_t pos;
  val = std::stoull(token, &pos);
  token = &buffer[pos];
}
