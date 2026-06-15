#include "../src/vm.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static int tests_run = 0;
static int tests_passed = 0;

#define EXPECT_INT(x) ((Data){.type = TYPE_INT, .word.i = (x)})

#define EXPECT_FLOAT(x) ((Data){.type = TYPE_FLOAT, .word.f = (x)})

typedef struct {
  const char *name;
  Inst *program;
  size_t size;

  Data expected;
  int expected_sp;
} TestCase;
/* Original Programs */

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

/* Mixed Int / Float Programs */

Inst prog_int_add[] = {PUSH(10), PUSH(3), ADD, HALT};

Inst prog_float_add[] = {PUSH_F(1.5), PUSH_F(2.5), ADD, HALT};

Inst prog_int_mul[] = {PUSH(6), PUSH(7), MUL, HALT};

Inst prog_float_div[] = {PUSH_F(3.0), PUSH_F(2.0), DIV, HALT};

Inst prog_int_mod[] = {PUSH(10), PUSH(3), MOD, HALT};

Inst prog_dup[] = {PUSH(5), DUP, HALT};

Inst prog_swap[] = {PUSH(1), PUSH(2), SWAP, HALT};

Inst prog_indup[] = {PUSH(1), PUSH(2), PUSH(3), INDUP(1), HALT};

Inst prog_cmp_eq_true[] = {PUSH(10), PUSH(10), CMP_EQ, HALT};

Inst prog_cmp_eq_false[] = {PUSH(10), PUSH(20), CMP_EQ, HALT};

Inst prog_cmp_lt_true[] = {PUSH(10), PUSH(20), CMP_LT, HALT};

Inst prog_cmp_gt_false[] = {PUSH(10), PUSH(20), CMP_GT, HALT};

Inst prog_cmp_eq_float[] = {PUSH_F(1.5), PUSH_F(1.5), CMP_EQ, HALT};
Inst prog_jmp[] = {
    PUSH(1),  // 0
    JMP(4),   // 1
    PUSH(99), // 2
    HALT,     // 3
    HALT      // 4
};
Inst prog_zjmp_taken[] = {
    PUSH(0), // 0
    ZJMP(4), // 1
    PUSH(1), // 2
    HALT,    // 3
    HALT     // 4
};
Inst prog_zjmp_not_taken[] = {
    PUSH(1),  // 0
    ZJMP(4),  // 1
    PUSH(99), // 2
    HALT,     // 3
    HALT      // 4
};
Inst prog_square[] = {
    PUSH(4), // 0
    CALL(3), // 1
    HALT,    // 2

    DUP, // 3
    MUL, // 4
    RET  // 5
};
Inst prog_fib5[] = {PUSH(5),   PUSH(2),   PUSH(0), PUSH(1),

                    INDUP(1),  INDUP(1),  ADD,

                    INDUP(3),  PUSH(1),   ADD,

                    INDUP(5),  SUB,

                    ZJMP(22),

                    INDUP(3),  PUSH(1),   ADD,

                    INSWAP(4), POP,

                    SWAP,      INSWAP(2), POP,

                    JMP(4),

                    HALT};
/* Test Table */

// TestCase tests[] = {
//     {"push", prog_push, sizeof(prog_push) / sizeof(prog_push[0]),
//      EXPECT_INT(42)},

//     {"add", prog_add, sizeof(prog_add) / sizeof(prog_add[0]),
//     EXPECT_INT(30)},

//     {"sub", prog_sub, sizeof(prog_sub) / sizeof(prog_sub[0]), EXPECT_INT(7)},

//     {"mul", prog_mul, sizeof(prog_mul) / sizeof(prog_mul[0]),
//     EXPECT_INT(42)},

//     {"div", prog_div, sizeof(prog_div) / sizeof(prog_div[0]), EXPECT_INT(5)},

//     {"mod", prog_mod, sizeof(prog_mod) / sizeof(prog_mod[0]), EXPECT_INT(1)},

//     {"pop", prog_pop, sizeof(prog_pop) / sizeof(prog_pop[0]), EXPECT_INT(2)},

//     {"chained", prog_chain, sizeof(prog_chain) / sizeof(prog_chain[0]),
//      EXPECT_INT(49)},

//     {"negative add", prog_neg_add,
//      sizeof(prog_neg_add) / sizeof(prog_neg_add[0]), EXPECT_INT(-2)},

//     {"negative sub", prog_neg_sub,
//      sizeof(prog_neg_sub) / sizeof(prog_neg_sub[0]), EXPECT_INT(-7)},

//     {"int add", prog_int_add, sizeof(prog_int_add) / sizeof(prog_int_add[0]),
//      EXPECT_INT(13)},

//     {"float add", prog_float_add,
//      sizeof(prog_float_add) / sizeof(prog_float_add[0]), EXPECT_FLOAT(4.0)},

//     {"int mul", prog_int_mul, sizeof(prog_int_mul) / sizeof(prog_int_mul[0]),
//      EXPECT_INT(42)},

//     {"float div", prog_float_div,
//      sizeof(prog_float_div) / sizeof(prog_float_div[0]), EXPECT_FLOAT(1.5)},

//     {"int mod", prog_int_mod, sizeof(prog_int_mod) / sizeof(prog_int_mod[0]),
//      EXPECT_INT(1)},
//     {"dup", prog_dup, sizeof(prog_dup) / sizeof(prog_dup[0]), EXPECT_INT(5)},

//     {"swap", prog_swap, sizeof(prog_swap) / sizeof(prog_swap[0]),
//      EXPECT_INT(1)},

//     {"indup", prog_indup, sizeof(prog_indup) / sizeof(prog_indup[0]),
//      EXPECT_INT(2)},

//     {"cmp eq true", prog_cmp_eq_true,
//      sizeof(prog_cmp_eq_true) / sizeof(prog_cmp_eq_true[0]), EXPECT_INT(1)},

//     {"cmp eq false", prog_cmp_eq_false,
//      sizeof(prog_cmp_eq_false) / sizeof(prog_cmp_eq_false[0]),
//      EXPECT_INT(0)},

//     {"cmp lt true", prog_cmp_lt_true,
//      sizeof(prog_cmp_lt_true) / sizeof(prog_cmp_lt_true[0]), EXPECT_INT(1)},

//     {"cmp gt false", prog_cmp_gt_false,
//      sizeof(prog_cmp_gt_false) / sizeof(prog_cmp_gt_false[0]),
//      EXPECT_INT(0)},

//     {"cmp eq float", prog_cmp_eq_float,
//      sizeof(prog_cmp_eq_float) / sizeof(prog_cmp_eq_float[0]),
//      EXPECT_INT(1)},
//     {"jmp", prog_jmp, sizeof(prog_jmp) / sizeof(prog_jmp[0]), EXPECT_INT(1),
//     1},

//     {"zjmp taken", prog_zjmp_taken,
//      sizeof(prog_zjmp_taken) / sizeof(prog_zjmp_taken[0]),
//      EXPECT_INT(0), // ignored
//      0},

//     {"zjmp not taken", prog_zjmp_not_taken,
//      sizeof(prog_zjmp_not_taken) / sizeof(prog_zjmp_not_taken[0]),
//      EXPECT_INT(99), 2},

//     {"square call", prog_square, sizeof(prog_square) /
//     sizeof(prog_square[0]),
//      EXPECT_INT(16), 1},
//     {"fib 5", prog_fib5, sizeof(prog_fib5) / sizeof(prog_fib5[0]),
//      EXPECT_INT(3)}};

TestCase tests[] = {
    {"push", prog_push, sizeof(prog_push) / sizeof(prog_push[0]),
     EXPECT_INT(42), 1},

    {"add", prog_add, sizeof(prog_add) / sizeof(prog_add[0]), EXPECT_INT(30),
     1},

    {"sub", prog_sub, sizeof(prog_sub) / sizeof(prog_sub[0]), EXPECT_INT(7), 1},

    {"mul", prog_mul, sizeof(prog_mul) / sizeof(prog_mul[0]), EXPECT_INT(42),
     1},

    {"div", prog_div, sizeof(prog_div) / sizeof(prog_div[0]), EXPECT_INT(5), 1},

    {"mod", prog_mod, sizeof(prog_mod) / sizeof(prog_mod[0]), EXPECT_INT(1), 1},

    {"pop", prog_pop, sizeof(prog_pop) / sizeof(prog_pop[0]), EXPECT_INT(2), 2},

    {"chained", prog_chain, sizeof(prog_chain) / sizeof(prog_chain[0]),
     EXPECT_INT(49), 1},

    {"negative add", prog_neg_add,
     sizeof(prog_neg_add) / sizeof(prog_neg_add[0]), EXPECT_INT(-2), 1},

    {"negative sub", prog_neg_sub,
     sizeof(prog_neg_sub) / sizeof(prog_neg_sub[0]), EXPECT_INT(-7), 1},

    {"int add", prog_int_add, sizeof(prog_int_add) / sizeof(prog_int_add[0]),
     EXPECT_INT(13), 1},

    {"float add", prog_float_add,
     sizeof(prog_float_add) / sizeof(prog_float_add[0]), EXPECT_FLOAT(4.0), 1},

    {"int mul", prog_int_mul, sizeof(prog_int_mul) / sizeof(prog_int_mul[0]),
     EXPECT_INT(42), 1},

    {"float div", prog_float_div,
     sizeof(prog_float_div) / sizeof(prog_float_div[0]), EXPECT_FLOAT(1.5), 1},

    {"int mod", prog_int_mod, sizeof(prog_int_mod) / sizeof(prog_int_mod[0]),
     EXPECT_INT(1), 1},

    {"dup", prog_dup, sizeof(prog_dup) / sizeof(prog_dup[0]), EXPECT_INT(5), 2},

    {"swap", prog_swap, sizeof(prog_swap) / sizeof(prog_swap[0]), EXPECT_INT(1),
     2},

    {"indup", prog_indup, sizeof(prog_indup) / sizeof(prog_indup[0]),
     EXPECT_INT(2), 4},

    {"cmp eq true", prog_cmp_eq_true,
     sizeof(prog_cmp_eq_true) / sizeof(prog_cmp_eq_true[0]), EXPECT_INT(1), 1},

    {"cmp eq false", prog_cmp_eq_false,
     sizeof(prog_cmp_eq_false) / sizeof(prog_cmp_eq_false[0]), EXPECT_INT(0),
     1},

    {"cmp lt true", prog_cmp_lt_true,
     sizeof(prog_cmp_lt_true) / sizeof(prog_cmp_lt_true[0]), EXPECT_INT(1), 1},

    {"cmp gt false", prog_cmp_gt_false,
     sizeof(prog_cmp_gt_false) / sizeof(prog_cmp_gt_false[0]), EXPECT_INT(0),
     1},

    {"cmp eq float", prog_cmp_eq_float,
     sizeof(prog_cmp_eq_float) / sizeof(prog_cmp_eq_float[0]), EXPECT_INT(1),
     1},

    {"jmp", prog_jmp, sizeof(prog_jmp) / sizeof(prog_jmp[0]), EXPECT_INT(1), 1},

    {"zjmp taken", prog_zjmp_taken,
     sizeof(prog_zjmp_taken) / sizeof(prog_zjmp_taken[0]), EXPECT_INT(0), 0},

    {"zjmp not taken", prog_zjmp_not_taken,
     sizeof(prog_zjmp_not_taken) / sizeof(prog_zjmp_not_taken[0]),
     EXPECT_INT(99), 2},

    {"square call", prog_square, sizeof(prog_square) / sizeof(prog_square[0]),
     EXPECT_INT(16), 1},

    {"fib 5", prog_fib5, sizeof(prog_fib5) / sizeof(prog_fib5[0]),
     EXPECT_INT(3), 5}};

int main(void) {
  size_t num_tests = sizeof(tests) / sizeof(tests[0]);

  for (size_t i = 0; i < num_tests; i++) {
    TestCase *t = &tests[i];

    tests_run++;

    printf("\n=== TEST: %s ===\n", t->name);

    for (size_t j = 0; j < t->size; j++) {
      if (haveOperand(t->program[j].opcode)) {
        if (t->program[j].opcode == OP_PUSH_F) {
          printf("%s  %lf\n", opcode_to_string(t->program[j].opcode),
                 t->program[j].value.f);
        } else {
          printf("%s  %ld\n", opcode_to_string(t->program[j].opcode),
                 t->program[j].value.i);
        }
      } else {
        printf("%s\n", opcode_to_string(t->program[j].opcode));
      }
    }

    VM vm = vm_new(t->program, t->size);

    vm_run(&vm);

    Data got = vm.stack[vm.stack_pos - 1];

    int passed = 0;

    if (t->expected_sp == 0 && vm.stack_pos == 0) {
      passed = 1;
    } else {
      if (got.type == TYPE_INT && t->expected.type == TYPE_INT) {
        printf("expected: %ld\n", t->expected.word.i);
        printf("received: %ld\n", got.word.i);

        passed = (got.word.i == t->expected.word.i);
      } else if (got.type == TYPE_FLOAT && t->expected.type == TYPE_FLOAT) {

        printf("expected: %.6f\n", t->expected.word.f);
        printf("received: %.6f\n", got.word.f);

        passed = fabs(got.word.f - t->expected.word.f) < 1e-9;
      }
    }
    if (passed) {
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