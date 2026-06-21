#ifndef VM_H
#define VM_H
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_STACK 1024
#define MAX_PROGRAM 65536
#define NUM_REGS 16
#define HEAP_SIZE 4096
#define STRING_POOL_SIZE 4096

#define VM_PANIC(fmt, ...)                                                     \
  do {                                                                         \
    fprintf(stderr, "[vm] " fmt "\n", ##__VA_ARGS__);                          \
    exit(1);                                                                   \
  } while (0)

typedef enum {
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_PTR,
  TYPE_STR,
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
  OP_JMP, // control flow
  OP_ZJMP,
  OP_NZJMP,
  OP_CALL,
  OP_RET,
  OP_MOV_IMM, // registers
  OP_MOV_IMM_F,
  OP_MOV_TOP,
  OP_PUSH_REG,
  OP_ALLOC, // memory
  OP_DEALLOC,
  OP_WRITE,
  OP_READ,
  OP_PUSH_STR, // string
  OP_ITOF,     // TYPE CONVERSION
  OP_FTOI,
  OP_ITOC,
  OP_TOI,
  OP_TOF,
  OP_NATIVE, // native
  OP_HALT,   // special
} Opcode;

typedef enum {
  PRINT_INT,
  PRINT_FLOAT,
  PRINT_CHAR,
  PRINT_STR,
  PRINTLN,
  EXIT_VM,
} NativeFunction;

typedef struct {
  Opcode opcode;
  Word value;
  NativeFunction nativeEntry;
  const char *string_literal;
  int reg_index;
} Inst;

typedef struct {
  Data stack[MAX_STACK];
  int stack_pos;

  size_t return_stack[MAX_STACK];
  int rsp;
  size_t ip;

  Data registers[NUM_REGS];

  Data heap[HEAP_SIZE];
  bool heap_used[HEAP_SIZE];
  int64_t heap_block_used[HEAP_SIZE];

  char string_pool[STRING_POOL_SIZE];
  size_t string_pool_pos;

  Inst *program;
  size_t program_size;
} VM;

#define PUSH(x)                                                                \
  (Inst) { .opcode = OP_PUSH, .value.i = (x) }
#define PUSH_F(x)                                                              \
  (Inst) { .opcode = OP_PUSH_F, .value.f = (x) }

#define POP                                                                    \
  (Inst) { .opcode = OP_POP }
#define DUP                                                                    \
  (Inst) { .opcode = OP_DUP }
#define SWAP                                                                   \
  (Inst) { .opcode = OP_SWAP }

#define INDUP(x)                                                               \
  (Inst) { .opcode = OP_INDUP, .value.i = (x) }
#define INSWAP(x)                                                              \
  (Inst) { .opcode = OP_INSWAP, .value.i = (x) }

#define ADD                                                                    \
  (Inst) { .opcode = OP_ADD }
#define SUB                                                                    \
  (Inst) { .opcode = OP_SUB }
#define MUL                                                                    \
  (Inst) { .opcode = OP_MUL }
#define DIV                                                                    \
  (Inst) { .opcode = OP_DIV }
#define MOD                                                                    \
  (Inst) { .opcode = OP_MOD }

#define CMP_EQ                                                                 \
  (Inst) { .opcode = OP_CMP_EQ }
#define CMP_NE                                                                 \
  (Inst) { .opcode = OP_CMP_NE }
#define CMP_LT                                                                 \
  (Inst) { .opcode = OP_CMP_LT }
#define CMP_LE                                                                 \
  (Inst) { .opcode = OP_CMP_LE }
#define CMP_GT                                                                 \
  (Inst) { .opcode = OP_CMP_GT }
#define CMP_GE                                                                 \
  (Inst) { .opcode = OP_CMP_GE }

#define JMP(x)                                                                 \
  (Inst) { .opcode = OP_JMP, .value.i = (x) }
#define ZJMP(x)                                                                \
  (Inst) { .opcode = OP_ZJMP, .value.i = (x) }
#define NZJMP(x)                                                               \
  (Inst) { .opcode = OP_NZJMP, .value.i = (x) }

#define CALL(x)                                                                \
  (Inst) { .opcode = OP_CALL, .value.i = (x) }
#define RET                                                                    \
  (Inst) { .opcode = OP_RET }

#define MOV_IMM(r, v)                                                          \
  (Inst) { .opcode = OP_MOV_IMM, .value.i = (v), .reg_index = (r) }
#define MOV_IMM_F(r, v)                                                        \
  (Inst) { .opcode = OP_MOV_IMM_F, .value.f = (v), .reg_index = (r) }
#define MOV_TOP(r)                                                             \
  (Inst) { .opcode = OP_MOV_TOP, .reg_index = (r) }
#define PUSH_REG(r)                                                            \
  (Inst) { .opcode = OP_PUSH_REG, .reg_index = (r) }

#define ALLOC                                                                  \
  (Inst) { .opcode = OP_ALLOC }
#define DEALLOC                                                                \
  (Inst) { .opcode = OP_DEALLOC }
#define WRITE                                                                  \
  (Inst) { .opcode = OP_WRITE }
#define READ                                                                   \
  (Inst) { .opcode = OP_READ }
#define PUSH_STR(x)                                                            \
  (Inst) { .opcode = OP_PUSH_STR, .string_literal = (x) }

#define ITOF                                                                   \
  (Inst) { .opcode = OP_ITOF }
#define FTOI                                                                   \
  (Inst) { .opcode = OP_FTOI }
#define ITOC                                                                   \
  (Inst) { .opcode = OP_ITOC }
#define TOI                                                                    \
  (Inst) { .opcode = OP_TOI }
#define TOF                                                                    \
  (Inst) { .opcode = OP_TOF }

#define NATIVE(t) (Inst){.opcode = OP_NATIVE, .nativeEntry = (t)};

#define HALT                                                                   \
  (Inst) { .opcode = OP_HALT }

VM vm_new(Inst *program, size_t size);
void vm_run(VM *vm);
void vm_dump_stack(VM *vm); /* debug helper */
const char *opcode_to_string(Opcode opcode);
bool haveOperand(Opcode opcode);
bool isInt(const char *);
bool isFloat(const char *);
bool iskeyword(const char *);
char *mystrdup(const char *s);
#endif
