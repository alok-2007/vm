#include "../src/vm.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

static int tests_run = 0;
static int tests_passed = 0;

#define EXPECT_INT(x) ((Data){.type = TYPE_INT, .word.i = (x)})
#define EXPECT_FLOAT(x) ((Data){.type = TYPE_FLOAT, .word.f = (x)})
#define EXPECT_PTR(x) ((Data){.type = TYPE_PTR, .word.i = (x)})

/* expect_crash: when true, the test is expected to call VM_PANIC (exit(1)).
 * We can't catch exit() in-process without fork(), so these are marked
 * but skipped from automatic PASS/FAIL — see note at bottom of main(). */
typedef struct {
  const char *name;
  Inst *program;
  size_t size;

  Data expected;
  int expected_sp;
  int check_value; /* 0 = only check sp (used for zjmp-taken style cases) */
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

/* ── intentionally-crashing programs ──
 * These call VM_PANIC -> exit(1). Running them in the same process would
 * kill the test binary, so they are NOT included in the automatic table.
 * They're kept here as documentation / for manual one-off testing:
 *
 *   Inst prog_double_dealloc[] = {
 *       PUSH(4), ALLOC, DUP, DEALLOC, DEALLOC, HALT
 *   };   // expect: VM_PANIC "you are free unallocated address from dealloc"
 *
 *   Inst prog_invalid_read[] = { PUSH(123456), READ, HALT };
 *       // expect: VM_PANIC (out of heap bounds, or unallocated)
 *
 *   Inst prog_invalid_write[] = { PUSH(123456), PUSH(42), WRITE, HALT };
 *       // expect: VM_PANIC
 *
 *   Inst prog_oob_write[] = {
 *       PUSH(4), ALLOC,        // [addr], block of size 4
 *       PUSH(99),              // [addr, 99]
 *       SWAP,                  // [99, addr]   -- still a valid address, NOT
 * oob by itself WRITE, HALT
 *   };
 *   To actually test "out of bounds within a block" you'd need pointer
 *   arithmetic (addr+5) which we deliberately do NOT support yet (Phase 6
 *   decision: absolute addressing only, no TYPE_PTR + TYPE_INT). So true
 *   OOB-within-block isn't expressible in bytecode yet -- only "address
 *   that was never allocated" is testable, which is what prog_invalid_read/
 *   prog_invalid_write above already cover.
 *
 * To actually test these, run each individually via fork()+waitpid() and
 * assert the child exited with status 1, or run them as separate manual
 * `make crash-test` style binaries. Not wired into this table.
 */

/* ────────────────────────── Test Table ─────────────────────────────── */

TestCase tests[] = {
    {"push", prog_push, sizeof(prog_push) / sizeof(prog_push[0]),
     EXPECT_INT(42), 1, 1},

    {"add", prog_add, sizeof(prog_add) / sizeof(prog_add[0]), EXPECT_INT(30), 1,
     1},

    {"sub", prog_sub, sizeof(prog_sub) / sizeof(prog_sub[0]), EXPECT_INT(7), 1,
     1},

    {"mul", prog_mul, sizeof(prog_mul) / sizeof(prog_mul[0]), EXPECT_INT(42), 1,
     1},

    {"div", prog_div, sizeof(prog_div) / sizeof(prog_div[0]), EXPECT_INT(5), 1,
     1},

    {"mod", prog_mod, sizeof(prog_mod) / sizeof(prog_mod[0]), EXPECT_INT(1), 1,
     1},

    {"pop", prog_pop, sizeof(prog_pop) / sizeof(prog_pop[0]), EXPECT_INT(2), 2,
     1},

    {"chained", prog_chain, sizeof(prog_chain) / sizeof(prog_chain[0]),
     EXPECT_INT(49), 1, 1},

    {"negative add", prog_neg_add,
     sizeof(prog_neg_add) / sizeof(prog_neg_add[0]), EXPECT_INT(-2), 1, 1},

    {"negative sub", prog_neg_sub,
     sizeof(prog_neg_sub) / sizeof(prog_neg_sub[0]), EXPECT_INT(-7), 1, 1},

    {"int add", prog_int_add, sizeof(prog_int_add) / sizeof(prog_int_add[0]),
     EXPECT_INT(13), 1, 1},

    {"float add", prog_float_add,
     sizeof(prog_float_add) / sizeof(prog_float_add[0]), EXPECT_FLOAT(4.0), 1,
     1},

    {"int mul", prog_int_mul, sizeof(prog_int_mul) / sizeof(prog_int_mul[0]),
     EXPECT_INT(42), 1, 1},

    {"float div", prog_float_div,
     sizeof(prog_float_div) / sizeof(prog_float_div[0]), EXPECT_FLOAT(1.5), 1,
     1},

    {"int mod", prog_int_mod, sizeof(prog_int_mod) / sizeof(prog_int_mod[0]),
     EXPECT_INT(1), 1, 1},

    {"dup", prog_dup, sizeof(prog_dup) / sizeof(prog_dup[0]), EXPECT_INT(5), 2,
     1},

    {"swap", prog_swap, sizeof(prog_swap) / sizeof(prog_swap[0]), EXPECT_INT(1),
     2, 1},

    {"indup", prog_indup, sizeof(prog_indup) / sizeof(prog_indup[0]),
     EXPECT_INT(2), 4, 1},

    {"cmp eq true", prog_cmp_eq_true,
     sizeof(prog_cmp_eq_true) / sizeof(prog_cmp_eq_true[0]), EXPECT_INT(1), 1,
     1},

    {"cmp eq false", prog_cmp_eq_false,
     sizeof(prog_cmp_eq_false) / sizeof(prog_cmp_eq_false[0]), EXPECT_INT(0), 1,
     1},

    {"cmp lt true", prog_cmp_lt_true,
     sizeof(prog_cmp_lt_true) / sizeof(prog_cmp_lt_true[0]), EXPECT_INT(1), 1,
     1},

    {"cmp gt false", prog_cmp_gt_false,
     sizeof(prog_cmp_gt_false) / sizeof(prog_cmp_gt_false[0]), EXPECT_INT(0), 1,
     1},

    {"cmp eq float", prog_cmp_eq_float,
     sizeof(prog_cmp_eq_float) / sizeof(prog_cmp_eq_float[0]), EXPECT_INT(1), 1,
     1},

    {"jmp", prog_jmp, sizeof(prog_jmp) / sizeof(prog_jmp[0]), EXPECT_INT(1), 1,
     1},

    {"zjmp taken", prog_zjmp_taken,
     sizeof(prog_zjmp_taken) / sizeof(prog_zjmp_taken[0]), EXPECT_INT(0), 0,
     0}, /* check_value=0: only sp matters, nothing was pushed */

    {"zjmp not taken", prog_zjmp_not_taken,
     sizeof(prog_zjmp_not_taken) / sizeof(prog_zjmp_not_taken[0]),
     EXPECT_INT(99), 1, 1},

    {"square call", prog_square, sizeof(prog_square) / sizeof(prog_square[0]),
     EXPECT_INT(16), 1, 1},

    {"fib 5", prog_fib5, sizeof(prog_fib5) / sizeof(prog_fib5[0]),
     EXPECT_INT(3), 5, 1},

    {"mov imm", prog_mov_imm, sizeof(prog_mov_imm) / sizeof(prog_mov_imm[0]),
     EXPECT_INT(42), 1, 1},

    {"mov imm float", prog_mov_imm_f,
     sizeof(prog_mov_imm_f) / sizeof(prog_mov_imm_f[0]), EXPECT_FLOAT(3.5), 1,
     1},

    {"mov top", prog_mov_top, sizeof(prog_mov_top) / sizeof(prog_mov_top[0]),
     EXPECT_INT(10), 2, 1},

    {"register add", prog_reg_add,
     sizeof(prog_reg_add) / sizeof(prog_reg_add[0]), EXPECT_INT(3), 1, 1},

    {"register index 5", prog_reg_index_5,
     sizeof(prog_reg_index_5) / sizeof(prog_reg_index_5[0]), EXPECT_INT(99), 1,
     1},

    /* ── memory tests ── */

    {"alloc", prog_alloc, sizeof(prog_alloc) / sizeof(prog_alloc[0]),
     EXPECT_PTR(0), /* expect block-start index 0 on a fresh heap */
     1, 1},

    {"write then read", prog_write_read,
     sizeof(prog_write_read) / sizeof(prog_write_read[0]), EXPECT_INT(42), 1,
     1},

    {"alloc then dealloc", prog_alloc_dealloc,
     sizeof(prog_alloc_dealloc) / sizeof(prog_alloc_dealloc[0]),
     EXPECT_INT(0), /* ignored, check_value=0 */
     0, 0},

    {"two independent blocks", prog_two_blocks,
     sizeof(prog_two_blocks) / sizeof(prog_two_blocks[0]), EXPECT_INT(11), 2,
     1},
};

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

    VM vm = vm_new(t->program, t->size);
    vm_run(&vm);

    int passed = 1;

    /* always check stack pointer first */
    if (vm.stack_pos != t->expected_sp) {
      printf("FAIL: sp mismatch (expected %d, got %d)\n", t->expected_sp,
             vm.stack_pos);
      passed = 0;
    }

    if (passed && t->check_value) {
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