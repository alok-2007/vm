#include "serialize.h"
#include "vm.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool serialize_program(const Inst *program, size_t count,
                       const char *filepath) {
  FILE *f = fopen(filepath, "wb");
  if (!f) {
    return false;
  }
  uint64_t magic = 0x54564D31;
  fwrite(&magic, sizeof(magic), 1, f);

  uint64_t inst_count = (uint64_t)count;
  fwrite(&inst_count, sizeof(inst_count), 1, f);

  for (size_t i = 0; i < count; i++) {
    Inst inst = program[i];

    // Explicitly wipe out runtime pointer addresses before serialization
    // to keep binary streams completely consistent across platforms
    inst.string_literal = NULL;

    fwrite(&inst.opcode, sizeof(inst.opcode), 1, f);
    fwrite(&inst.value, sizeof(inst.value), 1, f);
    fwrite(&inst.nativeEntry, sizeof(inst.nativeEntry), 1, f);
    fwrite(&inst.reg_index, sizeof(inst.reg_index), 1, f);

    // Evaluate the original instruction slot from input array safely
    if ((program[i].opcode == OP_PUSH_STR || program[i].opcode == OP_LOAD_LIB ||
         program[i].opcode == OP_LOAD_FN) &&
        program[i].string_literal != NULL) {
      uint64_t len = (uint64_t)strlen(program[i].string_literal);
      fwrite(&len, sizeof(len), 1, f);
      fwrite(program[i].string_literal, 1, len, f);
    } else {
      uint64_t len = 0;
      fwrite(&len, sizeof(len), 1, f);
    }
  }
  fclose(f);
  return true;
}

Inst *deserialize_program(const char *filepath, int *out_count) {
  FILE *f = fopen(filepath, "rb");
  if (f == NULL) {
    return NULL;
  }

  uint64_t magic;
  fread(&magic, sizeof(magic), 1, f);
  if (magic != 0x54564D31) {
    printf("not right file\n");
    fclose(f);
    return NULL;
  }

  uint64_t inst_count;
  fread(&inst_count, sizeof(inst_count), 1, f);
  if (inst_count <= 0) {
    printf("inst_count is less than 1\n");
    fclose(f);
    return NULL;
  }

  Inst *pro = malloc(inst_count * sizeof(*pro));
  if (!pro) {
    fclose(f);
    return NULL;
  }

  size_t i = 0;
  for (; i < inst_count; i++) {
    fread(&pro[i].opcode, sizeof(pro[i].opcode), 1, f);
    fread(&pro[i].value, sizeof(pro[i].value), 1, f);
    fread(&pro[i].nativeEntry, sizeof(pro[i].nativeEntry), 1, f);
    fread(&pro[i].reg_index, sizeof(pro[i].reg_index), 1, f);
    pro[i].string_literal = NULL; // Guarantee it's clean by default

    uint64_t len;
    fread(&len, sizeof(len), 1, f);
    if (len > 0) {
      char *str = malloc(len + 1);
      if (str == NULL) {
        printf("string failed from deserializer\n");
        // Free previously allocated string literals to prevent leaks
        for (size_t k = 0; k < i; k++) {
          if (pro[k].string_literal)
            free((void *)pro[k].string_literal);
        }
        free(pro);
        fclose(f);
        return NULL;
      }
      fread(str, 1, len, f);
      str[len] = '\0'; // Enforce solid null termination alignment
      pro[i].string_literal = str;
    } else {
      if (pro[i].opcode == OP_PUSH_STR || pro[i].opcode == OP_LOAD_LIB ||
          pro[i].opcode == OP_LOAD_FN) {
        char empty[] = {'\0'};
        pro[i].string_literal = mystrdup(empty);
      } else {
        pro[i].string_literal = NULL;
      }
    }
  }
  fclose(f);

  if (i != inst_count) {
    VM_PANIC("from deserializer program count mismatch\n");
  }
  *out_count = (int)i;
  return pro;
}