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
  case OP_JMP:
    return "JMP";
  case OP_ZJMP:
    return "ZJMP";
  case OP_NZJMP:
    return "NZJMP";
  case OP_CALL:
    return "CALL";
  case OP_RET:
    return "RET";
  case OP_HALT:
    return "HALT";
  default:
    return "UNKNOWN";
  }
}

bool haveOperand(Opcode opcode) {
  if (opcode == OP_PUSH || opcode == OP_PUSH_F || opcode == OP_INDUP ||
      opcode == OP_INSWAP || opcode == OP_JMP || opcode == OP_ZJMP ||
      opcode == OP_NZJMP || opcode == OP_CALL) {
    return true;
  }
  return false;
};

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
  while (vm->ip < vm->program_size) {
    printf("ip=%zu\n", vm->ip);
    vm_dump_stack(vm);
    Data a, b;
    int64_t depth;
    bool cond;
    Inst inst = vm->program[vm->ip];
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
      vm->stack[vm->stack_pos] = vm->stack[vm->stack_pos - 1 - depth];
      vm->stack_pos++;
      break;
    case OP_INSWAP:
      depth = inst.value.i;
      if (depth <= 0) {
        VM_PANIC("invalid INSWAP depth");
      }
      if (depth > vm->stack_pos) {
        VM_PANIC("stack underflow from INSWAP");
      }
      Data top = vm->stack[vm->stack_pos - 1];
      Data other = vm->stack[vm->stack_pos - 1 - depth];
      vm->stack[vm->stack_pos - 1 - depth] = top;
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
    case OP_JMP:
      vm->ip = inst.value.i;
      continue;
    case OP_ZJMP:
      a = pop(vm);

      cond = (a.type == TYPE_INT) ? a.word.i == 0 : a.word.f == 0.0;
      if (cond) {
        vm->ip = inst.value.i;
        continue;
      }
      break;
    case OP_NZJMP:
      a = pop(vm);
      cond = (a.type == TYPE_INT) ? a.word.i != 0 : a.word.f != 0.0;
      if (cond) {
        vm->ip = inst.value.i;
        continue;
      }
      break;
    case OP_CALL:
      if (vm->rsp >= MAX_STACK) {
        VM_PANIC("return stack overflow");
      }
      vm->return_stack[vm->rsp++] = vm->ip + 1;
      vm->ip = inst.value.i;
      continue;
      break;
    case OP_RET:
      if (vm->rsp <= 0) {
        VM_PANIC("return stack underflow");
      }
      vm->ip = vm->return_stack[--vm->rsp];
      continue;
    case OP_HALT:
      return;
    default:
      VM_PANIC("unknown opcode %d at ip=%zu", inst.opcode, vm->ip);
      break;
    }
    vm->ip++;
  }
}

void vm_dump_stack(VM *vm) {
  printf("sp=%d\n", vm->stack_pos);

  for (int i = 0; i < vm->stack_pos; i++) {
    printf("[%d] ", i);

    if (vm->stack[i].type == TYPE_INT)
      printf("%ld\n", vm->stack[i].word.i);
    else
      printf("%f\n", vm->stack[i].word.f);
  }
}

// int main() {
//   Inst program[] = {PUSH(5), PUSH(4), ADD};
//   VM vm = vm_new(program, sizeof(program) / sizeof(Inst));
//   vm_run(&vm);
//   vm_dump_stack(&vm);
//   return 0;
// }