#ifndef VASM_H
#define VASM_H

#include "vm.h"

typedef enum {
  TOK_IDENT,
  TOK_INT,
  TOK_FLOAT,
  TOK_STRING,
  TOK_COMMA,
  TOK_EOF,
} TokenType;

typedef struct {
  TokenType type;
  char *text;
  int64_t int_val;
  double float_val;
  char *str_val;
  int line;
} Token;

typedef struct {
  Token *items;
  size_t size;
  size_t capacity;
} TokenList;

TokenList lex(const char *src);

Inst *parse(TokenList tokens, size_t *out_cout);

const char *read_source_from_disk(const char *path);

#endif