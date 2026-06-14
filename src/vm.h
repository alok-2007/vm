#ifndef VM_H
#define VM_H

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
  OP_PUSH = 0,
  OP_POP,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_HALT
} Opcode;

typedef struct {
  Opcode opcode;
  int64_t value;
} Inst;

typedef struct {
  int64_t stack[MAX_STACK];
  int stack_pos;
  Inst *program;
  size_t program_size;
} VM;

#define PUSH(x)                                                                \
  (Inst) { OP_PUSH, (x) }
#define POP (Inst){OP_POP, 0}
#define ADD (Inst){OP_ADD, 0}
#define SUB (Inst){OP_SUB, 0}
#define MUL (Inst){OP_MUL, 0}
#define DIV (Inst){OP_DIV, 0}
#define MOD (Inst){OP_MOD, 0}
#define HALT (Inst){OP_HALT, 0}

VM vm_new(Inst *program, size_t size);
void vm_run(VM *vm);
void vm_dump_stack(VM *vm); /* debug helper */
const char *opcode_to_string(Opcode opcode);
#endif
