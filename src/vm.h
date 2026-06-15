#ifndef VM_H
#define VM_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_STACK 1024
#define MAX_PROGRAM 65536

#define VM_PANIC(fmt, ...)                                                     \
  do {                                                                         \
    fprintf(stderr, "[vm] " fmt "\n", ##__VA_ARGS__);                          \
    exit(1);                                                                   \
  } while (0)

typedef enum {
  TYPE_INT,
  TYPE_FLOAT,
} DataType;

typedef union {
  int64_t i;
  double f;
} Word;

typedef struct {
  Word word;
  DataType type;
} Data;
typedef enum {
  OP_PUSH = 0, // stack manipulation
  OP_PUSH_F,
  OP_POP,
  OP_DUP,
  OP_SWAP,
  OP_INDUP,
  OP_INSWAP,
  OP_ADD, // arithmetic
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_CMP_EQ, // comparison
  OP_CMP_NE,
  OP_CMP_LT,
  OP_CMP_LE,
  OP_CMP_GT,
  OP_CMP_GE,
  OP_HALT, // special
} Opcode;

typedef struct {
  Opcode opcode;
  Word value;
} Inst;

typedef struct {
  Data stack[MAX_STACK];
  int stack_pos;
  Inst *program;
  size_t program_size;
} VM;

#define PUSH(x)                                                                \
  (Inst) { OP_PUSH, .value.i = (x) }
#define PUSH_F(x)                                                              \
  (Inst) { OP_PUSH_F, .value.f = (x) }
#define POP                                                                    \
  (Inst) { OP_POP, }
#define DUP                                                                    \
  (Inst) { OP_DUP }
#define SWAP                                                                   \
  (Inst) { OP_SWAP }
#define INDUP(x)                                                               \
  (Inst) { OP_INDUP, .value.i = (x) }
#define INSWAP(x)                                                              \
  (Inst) { OP_INSWAP, .value.i = (x) }
#define ADD                                                                    \
  (Inst) { OP_ADD, }
#define SUB                                                                    \
  (Inst) { OP_SUB, }
#define MUL                                                                    \
  (Inst) { OP_MUL, }
#define DIV                                                                    \
  (Inst) { OP_DIV, }
#define MOD                                                                    \
  (Inst) { OP_MOD, }
#define CMP_EQ                                                                 \
  (Inst) { OP_CMP_EQ }
#define CMP_NE                                                                 \
  (Inst) { OP_CMP_NE }
#define CMP_LT                                                                 \
  (Inst) { OP_CMP_LT }
#define CMP_LE                                                                 \
  (Inst) { OP_CMP_LE }
#define CMP_GT                                                                 \
  (Inst) { OP_CMP_GT }
#define CMP_GE                                                                 \
  (Inst) { OP_CMP_GE }
#define HALT                                                                   \
  (Inst) { OP_HALT, }

VM vm_new(Inst *program, size_t size);
void vm_run(VM *vm);
void vm_dump_stack(VM *vm); /* debug helper */
const char *opcode_to_string(Opcode opcode);
#endif
