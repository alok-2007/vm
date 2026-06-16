#include "../src/vm.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int tests_run = 0;
static int tests_passed = 0;

#define EXPECT_INT(x) ((Data){.type = TYPE_INT, .word.i = (x)})
#define EXPECT_FLOAT(x) ((Data){.type = TYPE_FLOAT, .word.f = (x)})
#define EXPECT_PTR(x) ((Data){.type = TYPE_PTR, .word.i = (x)})
#define EXPECT_STRING(x) ((Data){.type = TYPE_STR, .word.i = (x)})

/* expect_crash: when true, the test is expected to call VM_PANIC (exit(1)).
 * We can't catch exit() in-process without fork(), so these are marked
 * but skipped from automatic PASS/FAIL — see note at bottom of main(). */
typedef struct {
  const char *name;
  Inst *program;
  size_t size;

  Data expected;
  int expected_sp;

  bool needIsolation;
} TestCase;
/* ───────────────────────── Original Programs ───────────────────────── */

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

/* ──────────────────── Mixed Int / Float Programs ───────────────────── */

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
    DUP,     // 3
    MUL,     // 4
    RET      // 5
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

Inst prog_mov_imm[] = {MOV_IMM(0, 42), PUSH_REG(0), HALT};
Inst prog_mov_imm_f[] = {MOV_IMM_F(0, 3.5), PUSH_REG(0), HALT};
Inst prog_mov_top[] = {PUSH(10), MOV_TOP(1), PUSH_REG(1), HALT};
Inst prog_reg_add[] = {MOV_IMM(0, 1), MOV_IMM(1, 2), PUSH_REG(0),
                       PUSH_REG(1),   ADD,           HALT};
Inst prog_reg_index_5[] = {MOV_IMM(5, 99), PUSH_REG(5), HALT};

/* ───────────────────────── Memory Programs ─────────────────────────── */

/* push 4, alloc, halt — stack has one TYPE_PTR value, sp==1 */
Inst prog_alloc[] = {PUSH(4), ALLOC, HALT};

/* alloc(4), write 42 at addr, read it back
 * stack convention: value pushed first, then address, then WRITE
 *   push 4      -> size for alloc
 *   alloc       -> [addr]
 *   dup         -> [addr, addr]   keep one copy to read back later
 *   push 42     -> [addr, addr, 42]
 *   swap        -> [addr, 42, addr]   address must be on top for WRITE
 *   write       -> pops address, then pops value -> [addr]
 *   read        -> pops address, pushes heap[addr] -> [42]
 */
Inst prog_write_read[] = {
    PUSH(4),  // 0  size
    ALLOC,    // 1  [addr]
    DUP,      // 2  [addr, addr]
    PUSH(42), // 3  [addr, addr, 42]
    SWAP,     // 4  [addr, 42, addr]
    WRITE,    // 5  [addr]
    READ,     // 6  [42]
    HALT      // 7
};

/* alloc then immediately dealloc — must not crash, sp==0 after dealloc */
Inst prog_alloc_dealloc[] = {
    PUSH(4), // 0
    ALLOC,   // 1  [addr]
    DEALLOC, // 2  []
    HALT     // 3
};

/* allocate two separate blocks, write/read both independently to confirm
 * the allocator doesn't hand out overlapping addresses */
Inst prog_two_blocks[] = {
    PUSH(2),  // 0  size for block A
    ALLOC,    // 1  [addrA]
    DUP,      // 2  [addrA, addrA]
    PUSH(11), // 3  [addrA, addrA, 11]
    SWAP,     // 4  [addrA, 11, addrA]
    WRITE,    // 5  [addrA]

    PUSH(2),  // 6  size for block B
    ALLOC,    // 7  [addrA, addrB]
    DUP,      // 8  [addrA, addrB, addrB]
    PUSH(22), // 9  [addrA, addrB, addrB, 22]
    SWAP,     // 10 [addrA, addrB, 22, addrB]
    WRITE,    // 11 [addrA, addrB]

    READ, // 12 pops addrB -> [addrA, 22]
    SWAP, // 13 [22, addrA]
    READ, // 14 pops addrA -> [22, 11]
    HALT  // 15  top should be 11, sp==2
};

/* alloc/dealloc panic tests */

Inst prog_invalid_reg[] = {MOV_IMM(16, 2), HALT};

Inst prog_double_free[] = {PUSH(4), ALLOC, DUP, DEALLOC, DEALLOC, HALT};

Inst prog_read_invalid[] = {PUSH(123456), READ, HALT};

Inst prog_write_invalid[] = {PUSH(123456), PUSH(42), WRITE, HALT};

Inst prog_alloc_negative[] = {PUSH(-1), ALLOC, HALT};

Inst prog_alloc_zero[] = {PUSH(0), ALLOC, HALT};

Inst prog_push_str_hi[] = {PUSH_STR("hi"), HALT};

Inst prog_push_str_two[] = {PUSH_STR("hello"), PUSH_STR("world"), HALT};

Inst prog_push_str_empty[] = {PUSH_STR(""), HALT};

/* ────────────────────────── Test Table ─────────────────────────────── */

TestCase tests[] = {
    {"push", prog_push, sizeof(prog_push) / sizeof(prog_push[0]),
     EXPECT_INT(42), 1, false},

    {"add", prog_add, sizeof(prog_add) / sizeof(prog_add[0]), EXPECT_INT(30), 1,
     false},

    {"sub", prog_sub, sizeof(prog_sub) / sizeof(prog_sub[0]), EXPECT_INT(7), 1,
     false},

    {"mul", prog_mul, sizeof(prog_mul) / sizeof(prog_mul[0]), EXPECT_INT(42), 1,
     false},

    {"div", prog_div, sizeof(prog_div) / sizeof(prog_div[0]), EXPECT_INT(5), 1,
     false},

    {"mod", prog_mod, sizeof(prog_mod) / sizeof(prog_mod[0]), EXPECT_INT(1), 1,
     false},

    {"pop", prog_pop, sizeof(prog_pop) / sizeof(prog_pop[0]), EXPECT_INT(2), 2,
     false},

    {"chained", prog_chain, sizeof(prog_chain) / sizeof(prog_chain[0]),
     EXPECT_INT(49), 1, false},

    {"negative add", prog_neg_add,
     sizeof(prog_neg_add) / sizeof(prog_neg_add[0]), EXPECT_INT(-2), 1, false},

    {"negative sub", prog_neg_sub,
     sizeof(prog_neg_sub) / sizeof(prog_neg_sub[0]), EXPECT_INT(-7), 1, false},

    {"int add", prog_int_add, sizeof(prog_int_add) / sizeof(prog_int_add[0]),
     EXPECT_INT(13), 1, false},

    {"float add", prog_float_add,
     sizeof(prog_float_add) / sizeof(prog_float_add[0]), EXPECT_FLOAT(4.0), 1,
     false},

    {"int mul", prog_int_mul, sizeof(prog_int_mul) / sizeof(prog_int_mul[0]),
     EXPECT_INT(42), 1, false},

    {"float div", prog_float_div,
     sizeof(prog_float_div) / sizeof(prog_float_div[0]), EXPECT_FLOAT(1.5), 1,
     false},

    {"int mod", prog_int_mod, sizeof(prog_int_mod) / sizeof(prog_int_mod[0]),
     EXPECT_INT(1), 1, false},

    {"dup", prog_dup, sizeof(prog_dup) / sizeof(prog_dup[0]), EXPECT_INT(5), 2,
     false},

    {"swap", prog_swap, sizeof(prog_swap) / sizeof(prog_swap[0]), EXPECT_INT(1),
     2, false},

    {"indup", prog_indup, sizeof(prog_indup) / sizeof(prog_indup[0]),
     EXPECT_INT(2), 4, false},

    {"cmp eq true", prog_cmp_eq_true,
     sizeof(prog_cmp_eq_true) / sizeof(prog_cmp_eq_true[0]), EXPECT_INT(1), 1,
     false},

    {"cmp eq false", prog_cmp_eq_false,
     sizeof(prog_cmp_eq_false) / sizeof(prog_cmp_eq_false[0]), EXPECT_INT(0), 1,
     false},

    {"cmp lt true", prog_cmp_lt_true,
     sizeof(prog_cmp_lt_true) / sizeof(prog_cmp_lt_true[0]), EXPECT_INT(1), 1,
     false},

    {"cmp gt false", prog_cmp_gt_false,
     sizeof(prog_cmp_gt_false) / sizeof(prog_cmp_gt_false[0]), EXPECT_INT(0), 1,
     false},

    {"cmp eq float", prog_cmp_eq_float,
     sizeof(prog_cmp_eq_float) / sizeof(prog_cmp_eq_float[0]), EXPECT_INT(1), 1,
     false},

    {"jmp", prog_jmp, sizeof(prog_jmp) / sizeof(prog_jmp[0]), EXPECT_INT(1), 1,
     false},

    {"zjmp taken", prog_zjmp_taken,
     sizeof(prog_zjmp_taken) / sizeof(prog_zjmp_taken[0]), EXPECT_INT(0), 0,
     false}, /* check_value=0: only sp matters, nothing was pushed */

    {"zjmp not taken", prog_zjmp_not_taken,
     sizeof(prog_zjmp_not_taken) / sizeof(prog_zjmp_not_taken[0]),
     EXPECT_INT(99), 1, false},

    {"square call", prog_square, sizeof(prog_square) / sizeof(prog_square[0]),
     EXPECT_INT(16), 1, false},

    {"fib 5", prog_fib5, sizeof(prog_fib5) / sizeof(prog_fib5[0]),
     EXPECT_INT(3), 5, false},

    {"mov imm", prog_mov_imm, sizeof(prog_mov_imm) / sizeof(prog_mov_imm[0]),
     EXPECT_INT(42), 1, false},

    {"mov imm float", prog_mov_imm_f,
     sizeof(prog_mov_imm_f) / sizeof(prog_mov_imm_f[0]), EXPECT_FLOAT(3.5), 1,
     false},

    {"mov top", prog_mov_top, sizeof(prog_mov_top) / sizeof(prog_mov_top[0]),
     EXPECT_INT(10), 2, false},

    {"register add", prog_reg_add,
     sizeof(prog_reg_add) / sizeof(prog_reg_add[0]), EXPECT_INT(3), 1, false},

    {"register index 5", prog_reg_index_5,
     sizeof(prog_reg_index_5) / sizeof(prog_reg_index_5[0]), EXPECT_INT(99), 1,
     false},

    /* ── memory tests ── */

    {"alloc", prog_alloc, sizeof(prog_alloc) / sizeof(prog_alloc[0]),
     EXPECT_PTR(0), /* expect block-start index 0 on a fresh heap */
     1, false},

    {"write then read", prog_write_read,
     sizeof(prog_write_read) / sizeof(prog_write_read[0]), EXPECT_INT(42), 1,
     false},

    {"alloc then dealloc", prog_alloc_dealloc,
     sizeof(prog_alloc_dealloc) / sizeof(prog_alloc_dealloc[0]),
     EXPECT_INT(0), /* ignored, check_value=0 */
     0, false},

    {"two independent blocks", prog_two_blocks,
     sizeof(prog_two_blocks) / sizeof(prog_two_blocks[0]), EXPECT_INT(11), 2,
     false},
    {"invalid register", prog_invalid_reg,
     sizeof(prog_invalid_reg) / sizeof(prog_invalid_reg[0]), EXPECT_INT(0), 0,
     true},

    {"double dealloc", prog_double_free,
     sizeof(prog_double_free) / sizeof(prog_double_free[0]), EXPECT_INT(0), 0,
     true},

    {"read with non-pointer type", prog_read_invalid,
     sizeof(prog_read_invalid) / sizeof(prog_read_invalid[0]), EXPECT_INT(0), 0,
     true},

    {"write with non-pointer type", prog_write_invalid,
     sizeof(prog_write_invalid) / sizeof(prog_write_invalid[0]), EXPECT_INT(0),
     0, true},

    {"alloc negative size", prog_alloc_negative,
     sizeof(prog_alloc_negative) / sizeof(prog_alloc_negative[0]),
     EXPECT_INT(0), 0, true},

    {"alloc zero size", prog_alloc_zero,
     sizeof(prog_alloc_zero) / sizeof(prog_alloc_zero[0]), EXPECT_INT(0), 0,
     true},
    {"push str hi", prog_push_str_hi,
     sizeof(prog_push_str_hi) / sizeof(prog_push_str_hi[0]),
     EXPECT_STRING(0), // string offset 0
     1, false},

    {"push str two", prog_push_str_two,
     sizeof(prog_push_str_two) / sizeof(prog_push_str_two[0]),
     EXPECT_STRING(6), // offset of "world" if "hello\0" stored first
     2, false},

    {"push str empty", prog_push_str_empty,
     sizeof(prog_push_str_empty) / sizeof(prog_push_str_empty[0]),
     EXPECT_STRING(0), // offset 0
     1, false},
};

// run_expect_panic --- function to fun program that can crash
static bool run_expect_panic(Inst *program, size_t size) {
  pid_t pid = fork();
  if (pid == 0) {
    VM vm = vm_new(program, size);
    vm_run(&vm);
    exit(0);
  } else {
    int status;
    waitpid(pid, &status, 0);
    bool crashed = WIFEXITED(status) && WEXITSTATUS(status) == 1;
    printf("%s\n",
           crashed ? "PASS (panicked as expected)" : "FAIL (did not panic)");
    return crashed;
  }
}

/* ───────────────────────────── main ─────────────────────────────────── */

int main(void) {
  size_t num_tests = sizeof(tests) / sizeof(tests[0]);

  for (size_t i = 0; i < num_tests; i++) {
    TestCase *t = &tests[i];
    tests_run++;

    printf("\n=== TEST: %s ===\n", t->name);

    for (size_t j = 0; j < t->size; j++) {
      printf("%2zu: ", j);

      switch (t->program[j].opcode) {
      case OP_MOV_IMM:
        printf("%s r%d, %ld\n", opcode_to_string(t->program[j].opcode),
               t->program[j].reg_index, t->program[j].value.i);
        break;

      case OP_MOV_IMM_F:
        printf("%s r%d, %f\n", opcode_to_string(t->program[j].opcode),
               t->program[j].reg_index, t->program[j].value.f);
        break;

      case OP_MOV_TOP:
      case OP_PUSH_REG:
        printf("%s r%d\n", opcode_to_string(t->program[j].opcode),
               t->program[j].reg_index);
        break;

      default:
        if (haveOperand(t->program[j].opcode)) {
          if (t->program[j].opcode == OP_PUSH_F) {
            printf("%s %f\n", opcode_to_string(t->program[j].opcode),
                   t->program[j].value.f);
          } else {
            printf("%s %ld\n", opcode_to_string(t->program[j].opcode),
                   t->program[j].value.i);
          }
        } else {
          printf("%s\n", opcode_to_string(t->program[j].opcode));
        }
      }
    }
    int passed = 1;
    if (t->needIsolation) {
      bool result = run_expect_panic(t->program, t->size);
      if (!result) {
        passed = 0;
      }
    } else {
      VM vm = vm_new(t->program, t->size);
      vm_run(&vm);
      vm_dump_stack(&vm);
      /* always check stack pointer first */
      if (vm.stack_pos != t->expected_sp) {
        printf("FAIL: sp mismatch (expected %d, got %d)\n", t->expected_sp,
               vm.stack_pos);
        passed = 0;
      }

      if (passed && vm.stack_pos > 0) {
        if (vm.stack_pos <= 0) {
          printf("FAIL: expected a value on stack but sp=%d\n", vm.stack_pos);
          passed = 0;
        } else {
          Data got = vm.stack[vm.stack_pos - 1];

          if (got.type != t->expected.type) {
            printf("FAIL: type mismatch (expected type %d, got type %d)\n",
                   t->expected.type, got.type);
            passed = 0;
          } else if (got.type == TYPE_INT) {
            printf("expected: %ld\n", t->expected.word.i);
            printf("received: %ld\n", got.word.i);
            passed = (got.word.i == t->expected.word.i);
          } else if (got.type == TYPE_FLOAT) {
            printf("expected: %.6f\n", t->expected.word.f);
            printf("received: %.6f\n", got.word.f);
            passed = fabs(got.word.f - t->expected.word.f) < 1e-9;
          } else if (got.type == TYPE_PTR) {
            printf("expected ptr: %ld\n", t->expected.word.i);
            printf("received ptr: %ld\n", got.word.i);
            passed = (got.word.i == t->expected.word.i);
          } else if (got.type == TYPE_STR) {
            const char *got_str = &vm.string_pool[got.word.i];
            const char *expected_str = &vm.string_pool[t->expected.word.i];
            printf("expected: \"%s\"\n", expected_str);
            printf("received: \"%s\"\n", got_str);
            passed = (strcmp(got_str, expected_str) == 0);
          }
        }
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
  printf("\nNote: crash-expected programs (double dealloc, invalid\n");
  printf("read/write) are documented in comments above but not run\n");
  printf("automatically since VM_PANIC calls exit(1) and would kill\n");
  printf("this test binary. Run them manually or wire up fork()-based\n");
  printf("isolation if you want them automated.\n");

  return tests_run == tests_passed ? 0 : 1;
}