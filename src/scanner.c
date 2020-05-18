#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

Scanner scanner;


void initScanner(const char* source, int starting_line) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = starting_line;
}

#ifdef CC_FEATURES

Scanner getCurrentScanner() {
  return scanner;
}


void replaceCurrentScanner(Scanner new_scanner) {
  scanner = new_scanner;
}

#endif

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}


static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}


static bool isAtEnd() {
  return *scanner.current == '\0';
}


static char advance() {
  scanner.current++;
  return scanner.current[-1];
}


static char peek() {
  return *scanner.current;
}


static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}


static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;

  scanner.current++;
  return true;
}


static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;

  return token;
}


static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;

  return token;
}


static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;

      case '\n':
        scanner.line++;
        advance();
        break;

      case '/':
        if (peekNext() == '/') {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;

      default:
        return;
    }
  }
}


static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0
     ) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}


static TokenType identifierType() {
  switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
#ifdef CC_FEATURES
          case 'c': return checkKeyword(2, 2, "ho", TOKEN_ECHO);
#endif
          case 'l': return checkKeyword(2, 2, "se", TOKEN_ELSE);
#ifdef CC_FEATURES
          case 'x': return checkKeyword(2, 2, "it", TOKEN_EXIT);
#endif
        }
      }
      break;
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
          case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
        }
      }
      break;
    case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
          case 'r':
            if (scanner.current - scanner.start > 2) {
              switch(scanner.start[2]) {
#ifdef CC_FEATURES
                case 'a': return checkKeyword(3, 7, "nsclude", TOKEN_TRANSCLUDE);
#endif
                case 'u': return checkKeyword(3, 1, "e", TOKEN_TRUE);
              }
            }
            break;
        }
      }
      break;
    case 'u': return checkKeyword(1, 4, "nvar", TOKEN_UNVAR);
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}


static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();

  return makeToken(identifierType());
}


static Token number() {
  while (isDigit(peek())) advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}


static Token string(char delimiter) {
  while (peek() != delimiter && !isAtEnd()) {
    if (peek() == '\n') scanner.line++;

#ifdef CC_FEATURES
    if(peek() == '\\' && peekNext() == delimiter) {
      // Consuming the backslash now will cause the call to advance() below to
      // then consume the delimiter, allowing the loop check to continue.
      advance();
    }
#endif

    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}


Token scanToken() {
  skipWhitespace();

  scanner.start = scanner.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);

    case '!': // !=
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': // ==
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': // <=
      return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': // >=
      return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

    case '"': return string('"');
#ifdef CC_FEATURES
    case '\'': return string('\'');
#endif
  }

  return errorToken("Unexpected character.");
}

const char* token_types[] = {
  // Single-character tokens. (0-10)
  "TOKEN_LEFT_PAREN", "TOKEN_RIGHT_PAREN",
  "TOKEN_LEFT_BRACE", "TOKEN_RIGHT_BRACE",
  "TOKEN_COMMA", "TOKEN_DOT", "TOKEN_MINUS", "TOKEN_PLUS",
  "TOKEN_SEMICOLON", "TOKEN_SLASH", "TOKEN_STAR",

  // One or two character tokens. (11-18)
  "TOKEN_BANG", "TOKEN_BANG_EQUAL",
  "TOKEN_EQUAL", "TOKEN_EQUAL_EQUAL",
  "TOKEN_GREATER", "TOKEN_GREATER_EQUAL",
  "TOKEN_LESS", "TOKEN_LESS_EQUAL",

  // Literals. (19-21)
  "TOKEN_IDENTIFIER", "TOKEN_STRING", "TOKEN_NUMBER",

  // Keywords. (22-37)
  "TOKEN_AND", "TOKEN_CLASS", "TOKEN_ELSE", "TOKEN_FALSE",
  "TOKEN_FOR", "TOKEN_FUN", "TOKEN_IF", "TOKEN_NIL", "TOKEN_OR",
  "TOKEN_PRINT", "TOKEN_RETURN", "TOKEN_SUPER", "TOKEN_THIS",
  "TOKEN_TRUE", "TOKEN_VAR", "TOKEN_WHILE",

#ifdef CC_FEATURES
// ******************************REMEMBER******************************
// You MUST add each new token to the precedence list in the compiler!
// You MUST add each new token to the token list in scanner.c!
// ******************************REMEMBER******************************
  "TOKEN_EXIT",
  "TOKEN_ECHO",
  "TOKEN_TRANSCLUDE",
  "TOKEN_UNVAR",
#endif

  "TOKEN_ERROR",
  "TOKEN_EOF"
};
const char* token_type_to_string(TokenType type) {
  return token_types[type];
}
