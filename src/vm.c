#include "vm.h"
#include <stdint.h>

const char *opcode_to_string(Opcode opcode) {
  switch (opcode) {
  case OP_PUSH:
    return "PUSH";
  case OP_PUSH_F:
    return "PUSH_F";
  case OP_DUP:
    return "DUP";
  case OP_SWAP:
    return "SWAP";
  case OP_INDUP:
    return "INDUP";
  case OP_INSWAP:
    return "INSWAP";
  case OP_POP:
    return "POP";
  case OP_ADD:
    return "ADD";
  case OP_SUB:
    return "SUB";
  case OP_MUL:
    return "MUL";
  case OP_DIV:
    return "DIV";
  case OP_MOD:
    return "MOD";
  case OP_CMP_EQ:
    return "CMP_EQ";
  case OP_CMP_NE:
    return "CMP_NE";
  case OP_CMP_LT:
    return "CMP_LT";
  case OP_CMP_LE:
    return "CMP_LE";
  case OP_CMP_GT:
    return "CMP_GT";
  case OP_CMP_GE:
    return "CMP_GE";
  case OP_HALT:
    return "HALT";
  default:
    return "UNKNOWN";
  }
}

VM vm_new(Inst *program, size_t size) {
  VM vm = {0};
  vm.program = program;
  vm.program_size = size;
  return vm;
}

static void push(VM *vm, int64_t val) {
  if (vm->stack_pos >= MAX_STACK) {
    VM_PANIC("stack overflow (max %d)", MAX_STACK);
  }
  vm->stack[vm->stack_pos++] = (Data){.word.i = val, .type = TYPE_INT};
}

static void push_f(VM *vm, double val) {
  if (vm->stack_pos >= MAX_STACK) {
    VM_PANIC("stack overflow (max %d)", MAX_STACK);
  }
  vm->stack[vm->stack_pos++] = (Data){.word.f = val, .type = TYPE_FLOAT};
}

static Data pop(VM *vm) {
  if (vm->stack_pos <= 0) {
    VM_PANIC("stack underflow");
  }
  return vm->stack[--vm->stack_pos];
}

static Data peek(VM *vm) {
  if (vm->stack_pos <= 0) {
    VM_PANIC("stack underflow on peek");
  }

  return vm->stack[vm->stack_pos - 1];
}

void vm_run(VM *vm) {
  for (size_t ip = 0; ip < vm->program_size; ip++) {
    Data a, b;
    int64_t depth;
    Inst inst = vm->program[ip];
    switch (inst.opcode) {
    case OP_PUSH:
      push(vm, inst.value.i);
      break;
    case OP_PUSH_F:
      push_f(vm, inst.value.f);
      break;
    case OP_POP:
      a = pop(vm);
      break;
    case OP_DUP:
      a = peek(vm);
      if (a.type == TYPE_INT) {
        push(vm, a.word.i);
      } else {
        push_f(vm, a.word.f);
      }
      break;
    case OP_SWAP:
      b = pop(vm);
      a = pop(vm);
      if (b.type == TYPE_INT) {
        push(vm, b.word.i);
      } else {
        push_f(vm, b.word.f);
      }
      if (a.type == TYPE_INT) {
        push(vm, a.word.i);
      } else {
        push_f(vm, a.word.f);
      }
      break;
    case OP_INDUP:
      depth = inst.value.i;
      if (depth <= 0) {
        VM_PANIC("invalid INDUP depth");
      }
      if (depth > vm->stack_pos) {
        VM_PANIC("stack underflow from INDUP");
      }
      if (depth + vm->stack_pos > MAX_STACK) {
        VM_PANIC("stack overflow from INDUP");
      }
      vm->stack[vm->stack_pos++] = vm->stack[vm->stack_pos - 1 - depth];
      break;
    case OP_INSWAP:
      depth = inst.value.i;
      if (depth <= 1) {
        VM_PANIC("invalid INSWAP depth");
      }
      if (depth > vm->stack_pos) {
        VM_PANIC("stack underflow from INSWAP");
      }
      Data top = vm->stack[vm->stack_pos - 1];
      Data other = vm->stack[vm->stack_pos - depth];
      vm->stack[vm->stack_pos - depth] = top;
      vm->stack[vm->stack_pos - 1] = other;
      break;
    case OP_ADD:
      b = pop(vm);
      a = pop(vm);
      if (a.type != b.type) {
        VM_PANIC("type mismatch");
      }
      if (a.type == TYPE_INT) {
        push(vm, a.word.i + b.word.i);
      } else {
        push_f(vm, a.word.f + b.word.f);
      }
      break;
    case OP_SUB:
      b = pop(vm);
      a = pop(vm);
      if (a.type != b.type) {
        VM_PANIC("type mismatch");
      }
      if (a.type == TYPE_INT) {
        push(vm, a.word.i - b.word.i);
      } else {
        push_f(vm, a.word.f - b.word.f);
      }
      break;
    case OP_MUL:
      b = pop(vm);
      a = pop(vm);
      if (a.type != b.type) {
        VM_PANIC("type mismatch");
      }
      if (a.type == TYPE_INT) {
        push(vm, a.word.i * b.word.i);
      } else {
        push_f(vm, a.word.f * b.word.f);
      }
      break;
    case OP_DIV:
      b = pop(vm);
      if (b.type == TYPE_INT) {
        if (b.word.i == 0) {
          VM_PANIC("division by zero");
        }
      } else {
        if (b.word.f == 0.00) {
          VM_PANIC("division by zero");
        }
      }
      a = pop(vm);
      if (a.type != b.type) {
        VM_PANIC("type mismatch");
      }
      if (a.type == TYPE_INT) {
        push(vm, a.word.i / b.word.i);
      } else {
        push_f(vm, a.word.f / b.word.f);
      }
      break;
    case OP_MOD:
      b = pop(vm);
      if (b.type == TYPE_INT) {
        if (b.word.i == 0) {
          VM_PANIC("modulo  by zero");
        }
      } else {
        if (b.word.f == 0.00) {
          VM_PANIC("modulo  by zero");
        }
      }
      a = pop(vm);
      if (a.type != b.type) {
        VM_PANIC("type mismatch");
      }
      if (a.type == TYPE_INT) {
        push(vm, a.word.i % b.word.i);
      } else {
        push_f(vm, fmod(a.word.f, b.word.f));
      }
      break;
    case OP_CMP_EQ:
      b = pop(vm);
      a = pop(vm);
      if (b.type != a.type) {
        VM_PANIC("type mismatch");
      }
      if (b.type == TYPE_INT) {
        push(vm, a.word.i == b.word.i);
      } else {
        push(vm, a.word.f == b.word.f);
      }
      break;
    case OP_CMP_NE:
      b = pop(vm);
      a = pop(vm);
      if (b.type != a.type) {
        VM_PANIC("type mismatch");
      }
      if (b.type == TYPE_INT) {
        push(vm, a.word.i != b.word.i);
      } else {
        push(vm, a.word.f != b.word.f);
      }
      break;
    case OP_CMP_LT:
      b = pop(vm);
      a = pop(vm);
      if (b.type != a.type) {
        VM_PANIC("type mismatch");
      }
      if (b.type == TYPE_INT) {
        push(vm, a.word.i < b.word.i);
      } else {
        push(vm, a.word.f < b.word.f);
      }
      break;
    case OP_CMP_LE:
      b = pop(vm);
      a = pop(vm);
      if (b.type != a.type) {
        VM_PANIC("type mismatch");
      }
      if (b.type == TYPE_INT) {
        push(vm, a.word.i <= b.word.i);
      } else {
        push(vm, a.word.f <= b.word.f);
      }
      break;
    case OP_CMP_GT:
      b = pop(vm);
      a = pop(vm);
      if (b.type != a.type) {
        VM_PANIC("type mismatch");
      }
      if (b.type == TYPE_INT) {
        push(vm, a.word.i > b.word.i);
      } else {
        push(vm, a.word.f > b.word.f);
      }
      break;
    case OP_CMP_GE:
      b = pop(vm);
      a = pop(vm);
      if (b.type != a.type) {
        VM_PANIC("type mismatch");
      }
      if (b.type == TYPE_INT) {
        push(vm, a.word.i >= b.word.i);
      } else {
        push(vm, a.word.f >= b.word.f);
      }
      break;

    case OP_HALT:
      return;
    default:
      VM_PANIC("unknown opcode %d at ip=%zu", inst.opcode, ip);
      break;
    }
  }
}

void vm_dump_stack(VM *vm) {
  printf("stack [%d]:", vm->stack_pos);
  for (int i = 0; i < vm->stack_pos; i++) {
    if (vm->stack[i].type == TYPE_INT) {
      printf(" %ld", vm->stack[i].word.i);
    } else {
      printf(" %lf", vm->stack[i].word.f);
    }
  }
  printf("\n");
}

// int main() {
//   Inst program[] = {PUSH(5), PUSH(4), ADD};
//   VM vm = vm_new(program, sizeof(program) / sizeof(Inst));
//   vm_run(&vm);
//   vm_dump_stack(&vm);
//   return 0;
// }