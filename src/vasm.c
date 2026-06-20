#include "vasm.h"
#include "vm.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

void token_list_push(TokenList *list, Token tok) {
  if (list->size >= list->capacity) {
    list->capacity = list->capacity == 0 ? 8 : list->capacity * 2;
    list->items = realloc(list->items, list->capacity * sizeof(Token));
    if (!list->items) {
      VM_PANIC("realloc failed from lex");
    }
  }
  list->items[list->size++] = tok;
}

bool isNum(char c) {
  if ((c >= (int)'0' && c <= (int)'9') || (c == '.')) {
    return true;
  } else {
    return false;
  }
}

TokenList lex(const char *src) {
  TokenList list = {0};
  int i = 0;
  int len = strlen(src);
  int line = 1;

  while (i < len) {
    char c = src[i];
    if (c == ' ') {
      i++;
      continue;
    }
    if (c == '\n') {
      i++;
      line++;
      continue;
    }
    if (isalpha(c) && c != ',') {
      int start = i;
      while (i < len && src[i] != '\n' && src[i] != ' ' && src[i] != ',') {
        i++;
      }
      int stringLen = i - start;
      char *text = malloc(stringLen + 1);
      strncpy(text, src + start, stringLen);
      text[stringLen] = '\0';
      for (int k = 0; text[k]; k++) {
        text[k] = tolower((unsigned char)text[k]);
      }
      Token tok = {.type = TOK_IDENT, .text = text, .line = line};
      token_list_push(&list, tok);
      if (src[i] == '\n') {
        line++;
        i++;
        continue;
      } else {
        continue;
      }
    } else if (isNum(c) || c == '-' || c == '+') {
      bool isPositive = true;
      if (c == '-') {
        i++;
        isPositive = false;
      } else if (c == '+') {
        i++;
        isPositive = true;
      }
      char buffer[256];
      int bufPtr = 0;
      if (!isPositive) {
        buffer[bufPtr++] = '-';
      }
      while (i < len && src[i] != '\n' && src[i] != ' ' && isNum(src[i]) &&
             bufPtr < 255) {
        buffer[bufPtr++] = src[i++];
      }
      buffer[bufPtr] = '\0';
      if (isFloat(buffer)) {
        double num = atof(buffer);
        Token tok = {.type = TOK_FLOAT, .float_val = num, .line = line};
        token_list_push(&list, tok);
      } else {
        int64_t num = (int64_t)atoll(buffer);
        Token tok = {.type = TOK_INT, .int_val = num, .line = line};
        token_list_push(&list, tok);
      }
      if (src[i] == '\n') {
        i++;
        line++;
        continue;
      } else {
        continue;
      }
    } else if (c == '\"') {
      i++;
      int start = i;
      while (i < len && src[i] != '\n' && src[i] != '\"') {
        i++;
      }
      int stringLen = i - start;
      char *text = malloc(stringLen + 1);
      strncpy(text, src + start, stringLen);
      text[stringLen] = '\0';
      if (src[i] != '\"') {
        VM_PANIC("Lexical Error: Unterminated string literal at line %d", line);
      } else {
        i++;
        Token tok = {.type = TOK_STRING, .str_val = text, .line = line};
        token_list_push(&list, tok);
      }
      continue;
    } else if (c == ',') {
      Token tok = {.type = TOK_COMMA, .line = line};
      token_list_push(&list, tok);
      i++;
      continue;
    } else if (c == '\0') {
      i++;
      Token tok = {.type = TOK_EOF, .line = line};
      token_list_push(&list, tok);
      break;
    }
  }
  return list;
}

// ------------------ parser -----------------------------------

Inst *parse(TokenList list, size_t *out_count) {
  int i = 0;
  int size = list.size;
  Token *items = list.items;
  Inst *pro = malloc(list.size * sizeof(*pro));
  size_t proLen = 0;
  while (i < size) {
    if (items[i].type != TOK_IDENT) {
      VM_PANIC("from parser syntax error line no : %d around %s", items[i].line,
               items[i].text);
    }
    if (items[i].type == TOK_EOF) {
      break;
    } else if (strcmp(items[i].text, "push") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_PUSH, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "push_f") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_FLOAT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] =
          (Inst){.opcode = OP_PUSH_F, .value.f = items[i].float_val};
      i++;
    } else if (strcmp(items[i].text, "pop") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_POP};
    } else if (strcmp(items[i].text, "dup") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_DUP};
    } else if (strcmp(items[i].text, "swap") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_SWAP};
    } else if (strcmp(items[i].text, "indup") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_INDUP, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "inswap") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_INSWAP, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "add") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_ADD};

    } else if (strcmp(items[i].text, "sub") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_SUB};
    } else if (strcmp(items[i].text, "mul") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_MUL};
    } else if (strcmp(items[i].text, "div") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_DIV};
    } else if (strcmp(items[i].text, "mod") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_MOD};
    } else if (strcmp(items[i].text, "cmp_eq") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CMP_EQ};
    } else if (strcmp(items[i].text, "cmp_ne") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CMP_NE};
    } else if (strcmp(items[i].text, "cmp_lt") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CMP_LT};
    } else if (strcmp(items[i].text, "cmp_le") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CMP_LE};
    } else if (strcmp(items[i].text, "cmp_gt") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CMP_GT};
    } else if (strcmp(items[i].text, "cmp_ge") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CMP_GE};
    } else if (strcmp(items[i].text, "jmp") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_JMP, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "zjmp") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no: %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_ZJMP, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "nzjmp") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no: %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_NZJMP, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "call") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_CALL, .value.i = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "ret") == 0) {
      i++;
      pro[proLen++] = (Inst){.opcode = OP_RET};
    } else if (strcmp(items[i].text, "mov_imm") == 0) {
      if (!((i + 3) < size && items[i + 1].type == TOK_INT &&
            items[i + 2].type == TOK_COMMA && items[i + 3].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      pro[proLen++] = (Inst){.opcode = OP_MOV_IMM,
                             .reg_index = items[i + 1].int_val,
                             .value.i = items[i + 3].int_val};
      i += 4;
    } else if (strcmp(items[i].text, "mov_imm_f") == 0) {
      if (!((i + 3) < size && items[i + 1].type == TOK_INT &&
            items[i + 2].type == TOK_COMMA && items[i + 3].type == TOK_FLOAT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      pro[proLen++] = (Inst){.opcode = OP_MOV_IMM_F,
                             .reg_index = items[i + 1].int_val,
                             .value.f = items[i + 3].float_val};
      i += 4;
    } else if (strcmp(items[i].text, "mov_top") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no: %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] =
          (Inst){.opcode = OP_MOV_TOP, .reg_index = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "push_reg") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_INT)) {
        VM_PANIC("from parser syntax error line no : %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] =
          (Inst){.opcode = OP_PUSH_REG, .reg_index = items[i].int_val};
      i++;
    } else if (strcmp(items[i].text, "alloc") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_ALLOC};
      i++;
    } else if (strcmp(items[i].text, "dealloc") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_DEALLOC};
      i++;
    } else if (strcmp(items[i].text, "write") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_WRITE};
      i++;
    } else if (strcmp(items[i].text, "read") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_READ};
      i++;
    } else if (strcmp(items[i].text, "push_str") == 0) {
      if (!((i + 1) < size && items[i + 1].type == TOK_STRING)) {
        VM_PANIC("from parser syntax error line no: %d around %s",
                 items[i].line, items[i].text);
      }
      i++;
      pro[proLen++] = (Inst){.opcode = OP_PUSH_STR,
                             .string_literal = mystrdup(items[i].str_val)};
      i++;
    } else if (strcmp(items[i].text, "itof") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_ITOF};
      i++;

    } else if (strcmp(items[i].text, "ftoi") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_FTOI};
      i++;
    } else if (strcmp(items[i].text, "itoc") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_ITOC};
      i++;
    } else if (strcmp(items[i].text, "toi") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_TOI};
      i++;
    } else if (strcmp(items[i].text, "tof") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_TOF};
      i++;
    } else if (strcmp(items[i].text, "halt") == 0) {
      pro[proLen++] = (Inst){.opcode = OP_HALT};
      i++;
    } else if (strcmp(items[i].text, "native") == 0) {
      i++;

      if (!(i < size && items[i].type == TOK_IDENT)) {
        VM_PANIC("from parser syntax error line %d: 'native' expects a "
                 "function name",
                 items[i - 1].line);
      }
      if (strcmp(items[i].text, "print_int") == 0) {
        pro[proLen++] = (Inst){.opcode = OP_NATIVE, .nativeEntry = PRINT_INT};
        i++;
      } else if (strcmp(items[i].text, "print_float") == 0) {
        pro[proLen++] = (Inst){.opcode = OP_NATIVE, .nativeEntry = PRINT_FLOAT};
        i++;
      } else if (strcmp(items[i].text, "print_char") == 0) {
        pro[proLen++] = (Inst){.opcode = OP_NATIVE, .nativeEntry = PRINT_CHAR};
        i++;
      } else if (strcmp(items[i].text, "print_str") == 0) {
        pro[proLen++] = (Inst){.opcode = OP_NATIVE, .nativeEntry = PRINT_STR};
        i++;
      } else if (strcmp(items[i].text, "println") == 0) {
        pro[proLen++] = (Inst){.opcode = OP_NATIVE, .nativeEntry = PRINTLN};
        i++;
      } else if (strcmp(items[i].text, "exit_vm") == 0) {
        pro[proLen++] = (Inst){.opcode = OP_NATIVE, .nativeEntry = EXIT_VM};
        i++;
      } else {
        VM_PANIC("unknown native function from parser");
      }
    } else {
      VM_PANIC("from parser unknown keyword");
    }
  }
  *out_count = proLen;
  return pro;
}

// ------- read source from disk --------

const char *read_source_from_disk(const char *filePath) {
  FILE *fd = fopen(filePath, "rb");
  if (fd == NULL) {
    VM_PANIC("unable to read source file");
  }
  if (fseek(fd, 0, SEEK_END) != 0) {
    fclose(fd);
    VM_PANIC("failed to  seek file");
  }

  int64_t size = ftell(fd);
  if (size < 0) {
    fclose(fd);
    VM_PANIC("failed to determine file size");
  }
  fseek(fd, 0, SEEK_SET);
  char *source = malloc(size + 1);
  if (source == NULL) {
    fclose(fd);
    VM_PANIC("out of memory for source file allocation");
  }
  size_t bytesRead = fread(source, sizeof(char), size, fd);
  source[bytesRead] = '\0';

  fclose(fd);
  return source;
}