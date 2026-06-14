#include "../src/vm.h"
#include <stdint.h>
#include <stdio.h>

static int tests_run = 0;
static int tests_passed = 0;

typedef struct {
  const char *name;
  Inst *program;
  size_t size;
  int64_t expected_top;
} TestCase;

/* Programs */

Inst prog_push[] = {PUSH(42), HALT};

Inst prog_add[] = {PUSH(10), PUSH(20), ADD, HALT};

Inst prog_sub[] = {PUSH(10), PUSH(3), SUB, HALT};

Inst prog_mul[] = {PUSH(6), PUSH(7), MUL, HALT};

Inst prog_div[] = {PUSH(10), PUSH(2), DIV, HALT};

Inst prog_mod[] = {PUSH(10), PUSH(3), MOD, HALT};

Inst prog_pop[] = {PUSH(1), PUSH(2), PUSH(3), POP, HALT};

Inst prog_chain[] = {PUSH(3), PUSH(4), ADD, PUSH(10), PUSH(3), SUB, MUL, HALT};

Inst prog_neg_add[] = {PUSH(-5), PUSH(3), ADD, HALT};

Inst prog_neg_sub[] = {PUSH(3), PUSH(10), SUB, HALT};

/* Test Table */

TestCase tests[] = {
    {"push", prog_push, sizeof(prog_push) / sizeof(prog_push[0]), 42},
    {"add", prog_add, sizeof(prog_add) / sizeof(prog_add[0]), 30},
    {"sub", prog_sub, sizeof(prog_sub) / sizeof(prog_sub[0]), 7},
    {"mul", prog_mul, sizeof(prog_mul) / sizeof(prog_mul[0]), 42},
    {"div", prog_div, sizeof(prog_div) / sizeof(prog_div[0]), 5},
    {"mod", prog_mod, sizeof(prog_mod) / sizeof(prog_mod[0]), 1},
    {"pop", prog_pop, sizeof(prog_pop) / sizeof(prog_pop[0]), 2},
    {"chained", prog_chain, sizeof(prog_chain) / sizeof(prog_chain[0]), 49},
    {"negative add", prog_neg_add,
     sizeof(prog_neg_add) / sizeof(prog_neg_add[0]), -2},
    {"negative sub", prog_neg_sub,
     sizeof(prog_neg_sub) / sizeof(prog_neg_sub[0]), -7}};

int main(void) {
  size_t num_tests = sizeof(tests) / sizeof(tests[0]);

  for (size_t i = 0; i < num_tests; i++) {
    TestCase *t = &tests[i];

    tests_run++;

    printf("\n=== TEST: %s ===\n", t->name);

    for (size_t j = 0; j < t->size; j++) {
      printf("%s %ld\n", opcode_to_string(t->program[j].opcode),
             t->program[j].value);
    }

    VM vm = vm_new(t->program, t->size);

    vm_run(&vm);

    int64_t got = vm.stack[vm.stack_pos - 1];

    printf("expected: %ld\n", t->expected_top);
    printf("received: %ld\n", got);

    if (got == t->expected_top) {
      tests_passed++;
      printf("PASS\n");
    } else {
      printf("FAIL\n");
    }
  }

  printf("\n====================\n");
  printf("Tests Run    : %d\n", tests_run);
  printf("Tests Passed : %d\n", tests_passed);
  printf("Tests Failed : %d\n", tests_run - tests_passed);

  return tests_run == tests_passed ? 0 : 1;
}