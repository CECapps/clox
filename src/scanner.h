#ifndef clox_scanner_h
#define clox_scanner_h

typedef struct {
  const char* start;
  const char* current;
  int line;
} Scanner;

typedef enum {
  // Single-character tokens. (0-10)
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // One or two character tokens. (11-18)
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,

  // Literals. (19-21)
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

  // Keywords. (22-37)
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

// ******************************REMEMBER******************************
// You MUST add each new token to the precedence list in the compiler!
// ******************************REMEMBER******************************
#ifdef FEATURE_EXIT
  TOKEN_EXIT,
#endif
#ifdef FEATURE_ECHO
  TOKEN_ECHO,
#endif
  TOKEN_TRANSCLUDE,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  const char* start;
  int length;
  int line;
} Token;

void initScanner(const char* source, int starting_line);
Token scanToken();

Scanner getCurrentScanner();
void replaceCurrentScanner(Scanner new_scanner);

#endif
