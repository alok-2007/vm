
#define _POSIX_C_SOURCE 200809L
#include "vm.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>

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
  case OP_MOV_IMM:
    return "MOV_IMM";
  case OP_MOV_IMM_F:
    return "MOV_IMM_F";
  case OP_MOV_TOP:
    return "MOV_TOP";
  case OP_PUSH_REG:
    return "PUSH_REG";
  case OP_ALLOC:
    return "ALLOC";
  case OP_DEALLOC:
    return "DEALLOC";
  case OP_WRITE:
    return "WRITE";
  case OP_READ:
    return "READ";
  case OP_PUSH_STR:
    return "PUSH_STR";
  case OP_ITOF:
    return "ITOF";
  case OP_FTOI:
    return "FTOI";
  case OP_ITOC:
    return "ITOC";
  case OP_TOI:
    return "TOI";
  case OP_TOF:
    return "TOF";
  case OP_NATIVE:
    return "NATIVE";
  case OP_HALT:
    return "HALT";
  default:
    return "UNKNOWN";
  }
}

bool iskeyword(const char *ch) {
  if (strcasecmp(ch, "push") == 0 || strcasecmp(ch, "push_f") == 0 ||
      strcasecmp(ch, "pop") == 0 || strcasecmp(ch, "dup") == 0 ||
      strcasecmp(ch, "swap") == 0 || strcasecmp(ch, "indup") == 0 ||
      strcasecmp(ch, "inswap") == 0 || strcasecmp(ch, "add") == 0 ||
      strcasecmp(ch, "sub") == 0 || strcasecmp(ch, "mul") == 0 ||
      strcasecmp(ch, "div") == 0 || strcasecmp(ch, "mod") == 0 ||
      strcasecmp(ch, "cmp_eq") == 0 || strcasecmp(ch, "cmp_ne") == 0 ||
      strcasecmp(ch, "cop_lt") == 0 || strcasecmp(ch, "cmp_le") == 0 ||
      strcasecmp(ch, "cmp_gt") == 0 || strcasecmp(ch, "ge") == 0 ||
      strcasecmp(ch, "jmp") == 0 || strcasecmp(ch, "zjmp") == 0 ||
      strcasecmp(ch, "nzjmp") == 0 || strcasecmp(ch, "call") == 0 ||
      strcasecmp(ch, "ret") == 0 || strcasecmp(ch, "mov_imm") == 0 ||
      strcasecmp(ch, "mov_imm_f") == 0 || strcasecmp(ch, "mov_top") == 0 ||
      strcasecmp(ch, "push_reg") == 0 || strcasecmp(ch, "alloc") == 0 ||
      strcasecmp(ch, "dealloc") == 0 || strcasecmp(ch, "write") == 0 ||
      strcasecmp(ch, "read") == 0 || strcasecmp(ch, "push_str") == 0 ||
      strcasecmp(ch, "itof") == 0 || strcasecmp(ch, "ftoi") == 0 ||
      strcasecmp(ch, "itoc") == 0 || strcasecmp(ch, "toi") == 0 ||
      strcasecmp(ch, "tof") == 0 || strcasecmp(ch, "native") == 0 ||
      strcasecmp(ch, "halt") == 0) {
    return true;
  }
  return false;
}

char *mystrdup(const char *s) {
  size_t len = strlen(s) + 1;
  char *p = malloc(len);
  return p ? memcpy(p, s, len) : NULL;
}

bool haveOperand(Opcode opcode) {
  if (opcode == OP_PUSH || opcode == OP_PUSH_F || opcode == OP_INDUP ||
      opcode == OP_INSWAP || opcode == OP_JMP || opcode == OP_ZJMP ||
      opcode == OP_NZJMP || opcode == OP_CALL || opcode == OP_MOV_IMM ||
      opcode == OP_MOV_TOP || opcode == OP_PUSH_REG || opcode == OP_PUSH_STR ||
      opcode == OP_NATIVE) {
    return true;
  }
  return false;
};

bool isInt(const char *str) {
  int len = strlen(str);
  for (int i = 0; i < len; i++) {
    char c = str[i];
    if (!(c >= (int)'0' && c <= (int)'9')) {
      return false;
    }
  }
  return true;
}

bool isFloat(const char *buffer) {
  int len = strlen(buffer);
  for (int i = 0; i < len; i++) {
    if (buffer[i] == '.') {
      return true;
    }
  }
  return false;
};

static void validate_reg(VM *vm, int reg) {
  if (reg < 0 || reg >= NUM_REGS) {
    VM_ERROR(vm, "invalid register %d", reg);
  }
}

void vm_print_stack_trace(VM *vm) {
  if (vm->rsp == 0)
    return;
  fprintf(stderr, "stack trace:\n");
  for (int i = vm->rsp - 1; i >= 0; i--) {
    fprintf(stderr, "  [%d] called from instruction %zu (line %d)\n",
            vm->rsp - 1 - i,
            vm->return_stack[i].return_addr - 1, // -1 = the CALL itself
            vm->return_stack[i].line);
  }
}

static void validateHeapAllocation(VM *vm, int64_t address) {
  int64_t i = address;
  bool foundBlock = false;
  while (i > -1) {
    if (vm->heap_block_used[i] != 0) {
      foundBlock = true;
      break;
    }
    i--;
  }
  if (foundBlock == false) {
    VM_ERROR(vm, "unallocated block used");
  } else {
    int64_t usedBlock = vm->heap_block_used[i];
    if ((i + usedBlock - 1) >= address) {
      return;
    } else {
      VM_ERROR(vm, "unallocated block used");
    }
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
    VM_ERROR(vm, "stack overflow (max %d)", MAX_STACK);
  }
  vm->stack[vm->stack_pos++] = (Data){.word.i = val, .type = TYPE_INT};
}

static void push_f(VM *vm, double val) {
  if (vm->stack_pos >= MAX_STACK) {
    VM_ERROR(vm, "stack overflow (max %d)", MAX_STACK);
  }
  vm->stack[vm->stack_pos++] = (Data){.word.f = val, .type = TYPE_FLOAT};
}

static Data pop(VM *vm) {
  if (vm->stack_pos <= 0) {
    VM_ERROR(vm, "stack underflow");
  }
  return vm->stack[--vm->stack_pos];
}

static Data peek(VM *vm) {
  if (vm->stack_pos <= 0) {
    VM_ERROR(vm, "stack underflow on peek");
  }

  return vm->stack[vm->stack_pos - 1];
}

void vm_run(VM *vm) {
  while (vm->ip < vm->program_size) {
    Data a, b;
    int64_t depth;
    bool cond;
    int64_t i;
    Inst inst = vm->program[vm->ip];
    switch (inst.opcode) {
    case OP_PUSH:
      push(vm, inst.value.i);
      break;
    case OP_PUSH_F:
      push_f(vm, inst.value.f);
      break;
    case OP_POP:
      pop(vm);
      break;
    case OP_DUP:
      vm->stack[vm->stack_pos] = peek(vm);
      vm->stack_pos++;
      break;
    case OP_SWAP:
      b = pop(vm);
      a = pop(vm);
      vm->stack[vm->stack_pos] = b;
      vm->stack_pos++;
      vm->stack[vm->stack_pos] = a;
      vm->stack_pos++;
      break;
    case OP_INDUP:
      depth = inst.value.i;
      if (depth <= 0) {
        VM_ERROR(vm, "invalid INDUP depth");
      }
      if (depth > vm->stack_pos) {
        VM_ERROR(vm, "stack underflow from INDUP");
      }
      if (depth + vm->stack_pos > MAX_STACK) {
        VM_ERROR(vm, "stack overflow from INDUP");
      }
      vm->stack[vm->stack_pos] = vm->stack[vm->stack_pos - 1 - depth];
      vm->stack_pos++;
      break;
    case OP_INSWAP:
      depth = inst.value.i;
      if (depth <= 0) {
        VM_ERROR(vm, "invalid INSWAP depth");
      }
      if (depth > vm->stack_pos) {
        VM_ERROR(vm, "stack underflow from INSWAP");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
          VM_ERROR(vm, "division by zero");
        }
      } else {
        if (b.word.f == 0.00) {
          VM_ERROR(vm, "division by zero");
        }
      }
      a = pop(vm);
      if (a.type != b.type) {
        VM_ERROR(vm, "type mismatch");
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
          VM_ERROR(vm, "modulo  by zero");
        }
      } else {
        if (b.word.f == 0.00) {
          VM_ERROR(vm, "modulo  by zero");
        }
      }
      a = pop(vm);
      if (a.type != b.type) {
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "type mismatch");
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
        VM_ERROR(vm, "return stack overflow");
      }
      vm->return_stack[vm->rsp++] =
          (CallFrame){.return_addr = (vm->ip + 1), .line = inst.line};
      vm->ip = inst.value.i;
      continue;
      break;
    case OP_RET:
      if (vm->rsp <= 0) {
        VM_ERROR(vm, "return stack underflow");
      }
      vm->ip = vm->return_stack[--vm->rsp].return_addr;
      continue;
    case OP_MOV_IMM:
      validate_reg(vm, inst.reg_index);
      vm->registers[inst.reg_index] =
          (Data){.type = TYPE_INT, .word.i = inst.value.i};
      break;
    case OP_MOV_IMM_F:
      validate_reg(vm, inst.reg_index);
      vm->registers[inst.reg_index] =
          (Data){.type = TYPE_FLOAT, .word.f = inst.value.f};
      break;
    case OP_MOV_TOP:
      validate_reg(vm, inst.reg_index);
      vm->registers[inst.reg_index] = peek(vm);
      break;
    case OP_PUSH_REG:
      validate_reg(vm, inst.reg_index);
      if (vm->stack_pos >= MAX_STACK) {
        VM_ERROR(vm, "stack overflow");
      }
      vm->stack[vm->stack_pos++] = vm->registers[inst.reg_index];
      break;
    case OP_ALLOC:
      a = pop(vm);
      if (a.type == TYPE_PTR || a.type == TYPE_FLOAT) {
        VM_ERROR(vm, "type mismatch from alloc");
      }
      int64_t size = a.word.i;
      if (size <= 0) {
        VM_ERROR(vm, "invalid size from alloc");
      }
      if (vm->stack_pos > MAX_STACK) {
        VM_ERROR(vm, "stack overflow");
      }
      i = 0;
      bool isAlloc = false;
      while (i < HEAP_SIZE) {
        if (vm->heap_used[i]) {
          i += vm->heap_block_used[i] - 1;
          continue;
        }
        int64_t initial_index = i;
        int64_t freeCount = 0;
        while (i < HEAP_SIZE && !vm->heap_used[i] && !(freeCount >= size)) {
          freeCount++;
          i++;
        }
        if (freeCount >= size) {
          vm->heap_used[initial_index] = true;
          vm->heap_block_used[initial_index] = size;
          vm->stack[vm->stack_pos] =
              (Data){.type = TYPE_PTR, .word.i = initial_index};
          vm->stack_pos++;
          isAlloc = true;
          break;
        }
      }
      if (!isAlloc) {
        VM_ERROR(vm, "insufficient heap memory");
      }
      break;
    case OP_DEALLOC:
      a = pop(vm);
      if (a.type == TYPE_FLOAT || a.type == TYPE_INT) {
        VM_ERROR(vm, "type mismatch from dealloc");
      }
      int64_t address = a.word.i;
      if (address < 0) {
        VM_ERROR(vm, "heap memory underflow");
      }
      if (address >= HEAP_SIZE) {
        VM_ERROR(vm, "heap memory overflow");
      }
      if (!vm->heap_used[address]) {
        VM_ERROR(vm, "you are free unallocated address from dealloc");
      }
      if (vm->heap_block_used[address] == 0) {
        VM_ERROR(vm, "wrong starting address from dealloc");
      }
      int64_t allocSize = vm->heap_block_used[address];
      for (int i = address; i < address + allocSize; i++) {
        vm->heap_used[i] = false;
      }
      vm->heap_block_used[address] = 0;
      break;
    case OP_WRITE:
      a = pop(vm); // address
      if (a.type != TYPE_PTR) {
        VM_ERROR(vm, "type mismatch from write");
      }
      address = a.word.i;
      if (address < 0) {
        VM_ERROR(vm, "heap memory underflow");
      }
      if (address >= HEAP_SIZE) {
        VM_ERROR(vm, "heap memory overflow");
      }
      validateHeapAllocation(vm, address);
      vm->heap[address] = pop(vm);
      break;
    case OP_READ:
      a = pop(vm); // address
      if (a.type != TYPE_PTR) {
        VM_ERROR(vm, "type mismatch from write");
      }
      address = a.word.i;
      if (address < 0) {
        VM_ERROR(vm, "heap memory underflow");
      }
      if (address >= HEAP_SIZE) {
        VM_ERROR(vm, "heap memory overflow");
      }
      if (vm->stack_pos >= MAX_STACK) {
        VM_ERROR(vm, "stack overflow");
      }
      validateHeapAllocation(vm, address);
      vm->stack[vm->stack_pos] = vm->heap[address];
      vm->stack_pos++;
      break;
    case OP_PUSH_STR: {
      const char *string_literal = inst.string_literal;
      int len = strlen(string_literal);
      if (vm->stack_pos >= MAX_STACK) {
        VM_ERROR(vm, "stack overflow");
      }
      if (vm->string_pool_pos + len + 1 > STRING_POOL_SIZE) {
        VM_ERROR(vm, "string pool full");
      }
      memcpy(&vm->string_pool[vm->string_pool_pos], string_literal, len + 1);
      int64_t string_offset = vm->string_pool_pos;
      vm->string_pool_pos += len + 1;
      vm->stack[vm->stack_pos++] =
          (Data){.type = TYPE_STR, .word.i = string_offset};
      break;
    }
    case OP_ITOF:
      a = pop(vm);
      if (a.type != TYPE_INT) {
        VM_ERROR(vm, "type mismatch from ITOF");
      }
      push_f(vm, (double)a.word.i);
      break;
    case OP_FTOI:
      a = pop(vm);
      if (a.type != TYPE_FLOAT) {
        VM_ERROR(vm, "type mismatch from FTOI");
      }
      push(vm, (int64_t)a.word.f);
      break;
    case OP_ITOC:
      a = pop(vm);
      if (a.type != TYPE_INT) {
        VM_ERROR(vm, "type mismatch from ITOF");
      }
      int64_t value = a.word.i;
      if (value < 0 || value > 255) {
        VM_ERROR(vm, "value out of char range");
      }
      push(vm, value);
      break;
    case OP_TOI:
      a = pop(vm);
      if (a.type == TYPE_STR || a.type == TYPE_PTR) {
        VM_ERROR(vm, "type mismatch from TOI");
      } else if (a.type == TYPE_FLOAT) {
        push(vm, (int64_t)a.word.f);
      } else {
        push(vm, a.word.i);
      }
      break;
    case OP_TOF:
      a = pop(vm);
      if (a.type == TYPE_STR || a.type == TYPE_PTR) {
        VM_ERROR(vm, "type mismatch from TOF");
      } else if (a.type == TYPE_INT) {
        push_f(vm, (double)a.word.i);
      } else {
        push_f(vm, a.word.f);
      }
      break;
    case OP_NATIVE:
      if (inst.nativeEntry == PRINT_INT) {
        a = pop(vm);
        if (a.type != TYPE_INT) {
          VM_ERROR(vm, "from op_native type mismatch");
        }
        printf("%ld\n", a.word.i);
      } else if (inst.nativeEntry == PRINT_FLOAT) {
        a = pop(vm);
        if (a.type != TYPE_FLOAT) {
          VM_ERROR(vm, "from op_native type mismatch");
        }
        printf("%.7f\n", a.word.f);
      } else if (inst.nativeEntry == PRINT_CHAR) {
        a = pop(vm);
        if (a.type != TYPE_INT) {
          VM_ERROR(vm, "from op_native type mismatch");
        }
        if (a.word.i < 0 || a.word.i > 255) {
          VM_ERROR(vm, "from native print char out of bound");
        }
        putchar(a.word.i);
      } else if (inst.nativeEntry == PRINT_STR) {
        a = pop(vm);
        if (a.type != TYPE_STR) {
          VM_ERROR(vm, "from op_native type mismatch");
        }
        const char *str = &vm->string_pool[a.word.i];
        printf("%s\n", str);
      } else if (inst.nativeEntry == PRINTLN) {
        printf("\n");
      } else if (inst.nativeEntry == EXIT_VM) {
        a = pop(vm);
        if (a.type != TYPE_INT) {
          VM_ERROR(vm, "from op_native exit type mismatch");
        }
        exit(a.word.i);
      }
      break;
    case OP_LOAD_LIB: {
      const char *file = inst.string_literal;
      if (file == NULL) {
        VM_ERROR(vm, "from op_load_lib file not provided");
      }
      if (vm->stack_pos >= MAX_STACK) {
        VM_ERROR(vm, "stack overflow");
      }
      char absolute_file_path[PATH_MAX];
      snprintf(absolute_file_path, sizeof(absolute_file_path), "%s/%s", "./lib",
               file);
      void *handle_ptr = dlopen(absolute_file_path, RTLD_LAZY);
      if (handle_ptr == NULL) {
        VM_ERROR(vm, "dlopen failed: %s", dlerror());
      }
      int64_t handle = (int64_t)(uintptr_t)handle_ptr;
      vm->stack[vm->stack_pos++] = (Data){.type = TYPE_PTR, .word.i = handle};
      break;
    }
    case OP_LOAD_FN: {
      a = pop(vm);
      if (a.type != TYPE_PTR) {
        VM_ERROR(vm, "from op_load_fn lib ptr is  not in stack");
      }
      const char *fn_name = inst.string_literal;
      if (fn_name == NULL) {
        VM_ERROR(vm, "from op_load_fn name not provided");
      }
      void *fn_ptr = dlsym((void *)a.word.i, fn_name);
      if (fn_ptr == NULL) {
        VM_ERROR(vm, "dlsym failed: %s", dlerror());
      }
      int64_t fn = (int64_t)(uintptr_t)fn_ptr;
      vm->stack[vm->stack_pos++] = (Data){.type = TYPE_PTR, .word.i = fn};
      break;
    }
    case OP_CALL_NATIVE:
      a = pop(vm);
      if (a.type != TYPE_INT) {
        VM_ERROR(vm, "from call native one operate on int");
      }
      b = pop(vm);
      if (b.type != TYPE_PTR) {
        VM_ERROR(vm, "from call native function ptr is not provided");
      }
      int64_t (*fn)(int64_t) = (int64_t (*)(int64_t))(uintptr_t)b.word.i;
      int64_t result = fn(a.word.i);
      push(vm, result);
      break;
    case OP_HALT:
      return;
    default:
      VM_ERROR(vm, "unknown opcode %d at ip=%zu", inst.opcode, vm->ip);
      break;
    }
    vm->ip++;
  }
}

void vm_dump_stack(VM *vm) {
  printf("sp=%d\n", vm->stack_pos);
  for (int i = 0; i < vm->stack_pos; i++) {
    printf("[%d] ", i);
    switch (vm->stack[i].type) {
    case TYPE_INT:
      printf("%ld\n", vm->stack[i].word.i);
      break;
    case TYPE_FLOAT:
      printf("%f\n", vm->stack[i].word.f);
      break;
    case TYPE_PTR:
      printf("ptr(%ld)\n", vm->stack[i].word.i);
      break;
    case TYPE_STR: {
      int64_t string_offset = vm->stack[i].word.i;
      printf("\"%s\"\n", &vm->string_pool[string_offset]);
      break;
    }
    }
  }
}

// int main() {
//   Inst program[] = {PUSH(5), PUSH(4), ADD};
//   VM vm = vm_new(program, sizeof(program) / sizeof(Inst));
//   vm_run(&vm);
//   vm_dump_stack(&vm);
//   return 0;
// }