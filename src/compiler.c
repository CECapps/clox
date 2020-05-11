
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"
#include "memory.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;


typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;


/*
  "C’s syntax for function pointer types is so bad that I always hide it behind
  a typedef. I understand the intent behind the syntax—the whole “declaration
  reflects use” thing—but I think it was a failed syntactic experiment."
*/
typedef void (*ParseFn)(bool canAssign);


typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;


typedef struct {
  Token name;
  int depth;
} Local;


typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT
} FunctionType;


typedef struct Compiler {
  struct Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
} Compiler;


Parser parser;

Compiler* current = NULL;

Chunk* compilingChunk;


static Chunk* currentChunk() {
  return &current->function->chunk;
}


static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}


static void error(const char* message) {
  errorAt(&parser.previous, message);
}


static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}


static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}


static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}


static bool check(TokenType type) {
  return parser.current.type == type;
}


static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}


static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}


static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}


static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}


static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}


static void emitReturn() {
  emitByte(OP_NIL);
  emitByte(OP_RETURN);
}


static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}


static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}


static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}


static ObjFunction* endCompiler() {
  emitReturn();
  ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), function->name != NULL ? function->name->chars : "<script>");
  }
#endif

  current = current->enclosing;
  return function;
}


static void beginScope() {
  current->scopeDepth++;
}


static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    emitByte(OP_POP);
    current->localCount--;
  }
}


static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;

  if (type != TYPE_SCRIPT) {
    current->function->name = copyString(parser.previous.start, parser.previous.length);
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
}


/*
  "Instead, we wrap the lookup in a function. That lets us forward declare
  getRule() before the definition of binary(), and then define getRule() after
  the table. We’ll need a couple of other forward declarations to handle the
  fact that our grammar is recursive...
*/
static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);


static uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}


static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}


static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Cannot read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}


static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
}


static void declareVariable() {
  // Global variables are implicitly declared.
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      error("Variable with this name already declared in this scope.");
    }
  }

  addLocal(*name);
}


static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}


static void markInitialized() {
  if (current->scopeDepth == 0) return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}


static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}


static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        error("Cannot have more than 255 arguments.");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}


static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}


static void binary(bool canAssign) {
  // Remember the operator.
  TokenType operatorType = parser.previous.type;

  // Compile the right operand.
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT);    break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL);             break;
    case TOKEN_GREATER:       emitByte(OP_GREATER);           break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT);     break;
    case TOKEN_LESS:          emitByte(OP_LESS);              break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT);  break;
    case TOKEN_PLUS:          emitByte(OP_ADD);               break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT);          break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY);          break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE);            break;
    default:
      return; // Unreachable.
  }
}


static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}


static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL:   emitByte(OP_NIL); break;
    case TOKEN_TRUE:  emitByte(OP_TRUE); break;
    default:
      return; // Unreachable.
  }
}


static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}


static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}


static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

#ifdef FEATURE_STRING_BACKSLASH_ESCAPES

static bool isHex(char c) {
  return (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F') ||
         (c >= '0' && c <= '9');
}

static void string(bool canAssign) {
  // The new string is guaranteed to be at least as long as the previous string,
  // plus one for the null byte terminator.  If there is any interpolation,
  // this allocation will be larger than needed.  This should not matter.  I think.
  char *new_str = ALLOCATE(char, parser.previous.length + 1);
  int new_index = 0;
#ifdef FEATURE_STRING_SINGLE_QUOTED
  // If single-quoted strings are enabled, only perform backslash interpolation
  // on double-quoted strings.  If backslash interpolation is disabled, this
  // code doesn't even get run.
  bool in_double_quotes = parser.previous.start[0] == '"';
#else
  // Single-quoted strings are disabled, so we are absolutely in double quotes.
  bool in_double_quotes = true;
#endif
  // The index starts at 1 to cut off the opening quote, and ends at length - 1
  // to cut off the closing quote.
  for(int orig_index = 1; orig_index < (parser.previous.length - 1); orig_index++) {
    char c = parser.previous.start[orig_index];
    int chars_remaining = parser.previous.length - orig_index;
    if(c == '\\' && chars_remaining > 0) {
      if(in_double_quotes) {
        switch(parser.previous.start[orig_index + 1]) {
          case 'n':  c = '\n'; orig_index++; break;
          case 'r':  c = '\r'; orig_index++; break;
          case 't':  c = '\t'; orig_index++; break;
          case '"':  c = '"';  orig_index++; break;
          case '\\':           orig_index++; break;
          case 'x': {
            // After the "\x", we expect two hex characters.  Examine each.
            if(chars_remaining > 2) {
              char hex_left = parser.previous.start[orig_index + 2];
              char hex_right = parser.previous.start[orig_index + 3];
              if(isHex(hex_left) && isHex(hex_right)) {
                // Now that we know the next two characters are hex, glue them back
                // together into a new string as expected by strtol.
                // This is super duper hacky.
                char lolhex[3];
                lolhex[0] = hex_left;
                lolhex[1] = hex_right;
                lolhex[2] = '\0';
                // The value returned by strtol given two hex digits will never
                // exceed the storage capacity of c (char), so this is safe.
                c = strtol(lolhex, NULL, 16);
                // We've processed the "x" and then two hex digits, adjust accordingly.
                orig_index += 3;
                break;
              }
            }
          }
        }
#ifdef FEATURE_STRING_SINGLE_QUOTED
      } else if(!in_double_quotes && parser.previous.start[orig_index + 1] == '\'') {
        // We are not inside of a double-quoted string.  We know that single-quoted
        // strings are enabled, and we know we are inside of one.  Turn \' into '.
        c = '\'';
        orig_index++;
#endif
      }
    }
    new_str[new_index] = c;
    new_index++;
  }
  // We don't need to worry about adding a null byte here, it gets taken care of
  // while sticking the cstring into an ObjString.
  emitConstant(OBJ_VAL(copyString(new_str, new_index)));
}

#else // FEATURE_STRING_BACKSLASH_ESCAPES

// The +1 and -2 adjust for the quotes in the string stored in parser.previous
static void string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}

#endif

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}


static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}


static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG:  emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default:
      return; // Unreachable.
  }
}


ParseRule rules[] = {
  { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
  { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
  { NULL,     and_,    PREC_AND },        // TOKEN_AND
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
  { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
  { literal,  NULL,    PREC_NONE },       // TOKEN_NIL
  { NULL,     or_,     PREC_OR },         // TOKEN_OR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
  { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
#ifdef FEATURE_EXIT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EXIT
#endif
#ifdef FEATURE_ECHO
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ECHO - same as TOKEN_PRINT
#endif
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}


static ParseRule* getRule(TokenType type) {
  return &rules[type];
}


static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}


static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}


static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  // Compile the parameter list.
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Cannot have more than 255 parameters.");
      }

      uint8_t paramConstant = parseVariable("Expect parameter name.");
      defineVariable(paramConstant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

  // The body.
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  // Create the function object.
  ObjFunction* function = endCompiler();
  emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
}


static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}


static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}


static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OP_POP);
}


static void forStatement() {
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;

  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);

    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();

  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope();
}


static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_ELSE)) statement();
  patchJump(elseJump);
}


static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

#ifdef FEATURE_ECHO

static void echoStatement() {
  uint8_t arg_count = 0;
  do {
    expression();
    if (arg_count == 255) {
      error("Cannot have more than 255 arguments.");
    }
    arg_count++;
  } while (match(TOKEN_COMMA));
  consume(TOKEN_SEMICOLON, "Expect ';' after values.");
  emitBytes(OP_ECHO, arg_count);
}

#endif

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Cannot return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}


static void whileStatement() {
  int loopStart = currentChunk()->count;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  statement();

  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

#ifdef FEATURE_EXIT

static void exitStatement() {
  if (match(TOKEN_SEMICOLON)) {
    emitByte(OP_NIL);
    emitByte(OP_EXIT);
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after exit value.");
    emitByte(OP_EXIT);
  }
}

#endif


static void transcludeStatement() {
  return;
  // transclude "filename";
  advance();
  string(false);
  consume(TOKEN_SEMICOLON, "Expect ';' after transclude string.");
  // The string containing the data we care about has been turned into a
  // constant, and it has been placed into the current chunk after an
  // OP_CONSTANT instruction.  It should, therefore, be the latest element
  // put into the constants array in the current chunk.  Right?
  Value filename = current->function->chunk.constants.values[ current->function->chunk.constants.count -1 ];
  char* fn = AS_CSTRING(filename);
  printf("** transcludeStatement: fn=%s\n", fn);
  transclude(fn);
  return;
}


static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;

    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        // Do nothing.
        ;
    }

    advance();
  }
}


static void declaration() {
  if (match(TOKEN_FUN)) {
    funDeclaration();
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}


static void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
#ifdef FEATURE_EXIT
  } else if(match(TOKEN_EXIT)) {
    exitStatement();
#endif
#ifdef FEATURE_ECHO
  } else if(match(TOKEN_ECHO)) {
    echoStatement();
#endif
  } else if (false) { // match(TOKEN_TRANSCLUDE)) {
    transcludeStatement();
  } else {
    expressionStatement();
  }
}


ObjFunction* compile(const char* source, int starting_line) {
  initScanner(source, starting_line);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* function = endCompiler();
  return parser.hadError ? NULL : function;
}


void transclude(char* source) {
  return;
  printf("** transclude: calling getCurrentScanner\n");
  Scanner old_scanner = getCurrentScanner();
  printf("** transclude: calling initScanner\n");
  initScanner(source, 1);

  printf("** transclude: calling advance\n");
  advance();

  printf("** transclude: calling while\n");
  while (!match(TOKEN_EOF)) {
    printf("** transclude: declaration!\n");
    declaration();
  }

  printf("** transclude: calling replaceCurrentScanner\n");
  replaceCurrentScanner(old_scanner);

  printf("** transclude: done!\n");
}
