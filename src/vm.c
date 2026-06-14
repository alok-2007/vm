#include "vm.h"

const char *opcode_to_string(Opcode opcode) {
  switch (opcode) {
  case OP_PUSH:
    return "PUSH";
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
  vm->stack[vm->stack_pos++] = val;
}

static int64_t pop(VM *vm) {
  if (vm->stack_pos <= 0) {
    VM_PANIC("stack underflow");
  }
  return vm->stack[--vm->stack_pos];
}

static int64_t peek(VM *vm) {
  if (vm->stack_pos <= 0) {
    VM_PANIC("stack underflow on peek");
  }
  return vm->stack[vm->stack_pos - 1];
}

void vm_run(VM *vm) {
  for (size_t ip = 0; ip < vm->program_size; ip++) {
    int64_t a, b;
    Inst inst = vm->program[ip];
    switch (inst.opcode) {
    case OP_PUSH:
      a = inst.value;
      push(vm, a);
      break;
    case OP_POP:
      a = pop(vm);
      break;
    case OP_ADD:
      b = pop(vm);
      a = pop(vm);
      push(vm, a + b);
      break;
    case OP_SUB:
      b = pop(vm);
      a = pop(vm);
      push(vm, a - b);
      break;
    case OP_MUL:
      b = pop(vm);
      a = pop(vm);
      push(vm, a * b);
      break;
    case OP_DIV:
      b = pop(vm);
      if (b == 0) {
        VM_PANIC("division by zero");
      }
      a = pop(vm);
      push(vm, a / b);
      break;
    case OP_MOD:
      b = pop(vm);
      if (b == 0) {
        VM_PANIC("modulo by zero");
      }
      a = pop(vm);
      push(vm, a % b);
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
    printf(" %ld", vm->stack[i]);
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