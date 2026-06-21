// vasm.h
#ifndef VASM_H
#define VASM_H
#include "hashmap.h"
#include "vm.h"
#define IMPORT_STACK_MAX 256

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
char *preprocessor(const char *source, const char *base_dir,
                   const char ***import_stack, int stack_depth,
                   int *stack_capacity, HashMap *map);

#endif