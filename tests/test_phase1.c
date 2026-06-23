#define _POSIX_C_SOURCE 200809L
#include "../src/serialize.h"
#include "../src/vasm.h"
#include "../src/vm.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // For mkdir on Linux/macOS
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
  const char *name;
  Data data;
  bool willPanic;
  bool isStdOut;
  bool isExit;
  int exitCode;
  const char *stdOut;
  int expected_sp;
} Expected_Result;

const char *get_extension(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if (!dot || dot == filename) {
    return "";
  }
  return dot + 1;
}

const char *get_filename(char *filename) {
  char *dot = strrchr(filename, '.');
  if (!dot || dot == filename) {
    VM_PANIC("from get_filename no name");
  }
  char buffer[1024];
  int bufPtr = 0;
  while (filename < dot && bufPtr < 1023) {
    buffer[bufPtr++] = *filename;
    filename += 1;
  }
  buffer[bufPtr] = '\0';
  return mystrdup(buffer);
}

// run_expect_panic --- function to run a program that can crash
static bool run_expect_panic(Inst *program, size_t size,
                             Expected_Result *result) {
  pid_t pid = fork();
  if (pid == 0) {
    VM vm = vm_new(program, size);
    vm_run(&vm);
    exit(0);
  } else {
    int status;
    bool crashed;
    waitpid(pid, &status, 0);
    if (result->isExit) {
      crashed = (status == result->exitCode);
    } else {
      crashed = WIFEXITED(status) && WEXITSTATUS(status) == 1;
    }
    printf("%s\n",
           crashed ? "PASS (panicked as expected)" : "FAIL (did not panic)");
    return crashed;
  }
}

void buildExpectedHashmap(HashMap *map, const char *src) {
  int i = 0;
  while (src[i] != '\0') {
    // Skip empty lines or leading whitespace characters
    if (src[i] == ' ' || src[i] == '\r' || src[i] == '\n') {
      i++;
      continue;
    }

    Expected_Result *newResult = malloc(sizeof(*newResult));
    newResult->willPanic = false;
    newResult->isStdOut = false; // CRITICAL: Explicitly initialize booleans
    newResult->isExit = false;   // CRITICAL: Explicitly initialize booleans
    newResult->expected_sp = 0;
    newResult->stdOut = NULL;
    memset(&newResult->data, 0, sizeof(Data));

    char buffer[1024];
    int bufPtr = 0;

    // 1. Parse File Base Name
    while (src[i] != '\0' && src[i] != '\n' && src[i] != ' ') {
      buffer[bufPtr++] = src[i++];
    }
    buffer[bufPtr] = '\0';

    // Format target filename key with its proper extension format
    char filename_key[1024];
    snprintf(filename_key, sizeof(filename_key), "%s", buffer);
    newResult->name = mystrdup(filename_key);

    while (src[i] == ' ')
      i++;

    // 2. Parse Expected Output Token Type
    bufPtr = 0;
    while (src[i] != '\0' && src[i] != '\n' && src[i] != ' ') {
      buffer[bufPtr++] = src[i++];
    }
    buffer[bufPtr] = '\0';

    if (strcmp(buffer, "TYPE_INT") == 0) {
      newResult->data.type = TYPE_INT;
    } else if (strcmp(buffer, "TYPE_FLOAT") == 0) {
      newResult->data.type = TYPE_FLOAT;
    } else if (strcmp(buffer, "TYPE_PTR") == 0) {
      newResult->data.type = TYPE_PTR;
    } else if (strcmp(buffer, "TYPE_STR") == 0) {
      newResult->data.type = TYPE_STR;
    } else if (strcmp(buffer, "PANIC") == 0) {
      newResult->willPanic = true;
    } else if (strcmp(buffer, "TYPE_STDOUT") == 0) {
      newResult->isStdOut = true;
      while (src[i] == ' ')
        i++;
      if (src[i] != '\"') {
        VM_PANIC("unsupported stdout format");
      }
      i++; // Skip opening quote
      bufPtr = 0;

      // UNESCAPE LOOP: Process literal \n markers into real newline bytes
      while (src[i] != '\"' && src[i] != '\0') {
        if (src[i] == '\\' && src[i + 1] == 'n') {
          buffer[bufPtr++] = '\n';
          i += 2;
        } else {
          buffer[bufPtr++] = src[i++];
        }
      }
      if (src[i] != '\"') {
        VM_PANIC("unsupported stdout format");
      }
      i++; // Skip closing quote
      buffer[bufPtr] = '\0';
      newResult->stdOut = mystrdup(buffer);

    } else if (strcmp(buffer, "EXIT") == 0) {
      newResult->isExit = true;
      while (src[i] == ' ')
        i++;
      bufPtr = 0;
      while (src[i] != '\0' && src[i] != ' ' && src[i] != '\n') {
        buffer[bufPtr++] = src[i++];
      }
      buffer[bufPtr] = '\0';
      if (!isInt(buffer)) {
        VM_PANIC("unsupported exit code format");
      }
      newResult->exitCode = atoi(buffer);
    } else {
      VM_PANIC("unsupported type from hashmap loader: %s", buffer);
    }

    // Short circuit the line evaluation loop if it is an expected panic
    // condition
    if (newResult->willPanic || newResult->isStdOut || newResult->isExit) {
      while (src[i] != '\0' && src[i] != '\n') {
        i++;
      }
      hashmap_insert(map, newResult->name, newResult);
      continue;
    }

    while (src[i] == ' ')
      i++;

    // 3. Parse Expected Top Data Word Value
    bufPtr = 0;
    while (src[i] != '\0' && src[i] != '\n' && src[i] != ' ') {
      buffer[bufPtr++] = src[i++];
    }
    buffer[bufPtr] = '\0';

    if (newResult->data.type == TYPE_FLOAT) {
      newResult->data.word.f = atof(buffer);
    } else {
      newResult->data.word.i = atoll(buffer);
    }

    while (src[i] == ' ')
      i++;

    // 4. Parse Expected Stack Pointer Size
    bufPtr = 0;
    while (src[i] != '\0' && src[i] != '\n' && src[i] != ' ') {
      buffer[bufPtr++] = src[i++];
    }
    buffer[bufPtr] = '\0';
    newResult->expected_sp = atoi(buffer);

    // Consume trailing elements up to the newline boundary
    while (src[i] != '\0' && src[i] != '\n') {
      i++;
    }

    hashmap_insert(map, newResult->name, newResult);
  }
}
// -- serializer --
void serializer(const char *dirSrc) {
  DIR *dir = opendir(dirSrc);
  if (dir == NULL) {
    VM_PANIC("unable to open directory");
  }
  if (mkdir("tests/tasms", 0777) != 0) {
    if (errno != EEXIST) {
      VM_PANIC("failed to create tasms directory");
    }
  }
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    const char *ext = get_extension(entry->d_name);
    if (strcmp(ext, "vasm") != 0) {
      continue; // skip .hasm files (and anything else) — only run .vasm as
                // top-level tests
    }
    char absolute_file_path[PATH_MAX];
    snprintf(absolute_file_path, sizeof(absolute_file_path), "%s/%s", dirSrc,
             entry->d_name);
    const char *rawSrc = read_source_from_disk(absolute_file_path);
    printf("rawSrc: \n%s\n", rawSrc);
    HashMap *def_Map = hashmap_new(100);
    int stack_capacity = 16;
    const char **import_stack = malloc(stack_capacity * sizeof(char *));
    import_stack[0] = mystrdup(absolute_file_path);
    const char *src = preprocessor(rawSrc, dirSrc, &import_stack, 1,
                                   &stack_capacity, def_Map);
    printf("processed:\n %s\n", src);
    TokenList tokens = lex(src);
    size_t program_size = 0;
    Inst *program = parse(tokens, &program_size);
    if (program_size <= 0) {
      VM_PANIC("parse didn't return program");
    }
    char absolute_tasm_path[PATH_MAX];
    snprintf(absolute_tasm_path, sizeof(absolute_tasm_path), "%s/%s.vbc",
             "tests/tasms", get_filename(entry->d_name));
    bool serialize_res =
        serialize_program(program, program_size, absolute_tasm_path);
  }
}

/* ───────────────────────────── main ─────────────────────────────────── */
int main(int argc, char *argv[]) {
  if (argc != 3) {
    VM_PANIC("usage - : ./test [testcaseDir] [expected.txt]");
  }
  int tests_run = 0;
  int tests_passed = 0;

  const char *testCaseDir = argv[1];
  const char *expectedFile = argv[2];
  serializer(testCaseDir);
  HashMap *map = hashmap_new(100);
  printf("in hashmp build\n");
  buildExpectedHashmap(map, read_source_from_disk(expectedFile));
  printf("out hashmp build\n");

  DIR *dir = opendir("tests/tasms");
  if (dir == NULL) {
    VM_PANIC("unable to open directory");
  }

  struct dirent *entry;
  printf("--BEGIN TEST--\n");
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    const char *ext = get_extension(entry->d_name);
    if (strcmp(ext, "tasm") != 0) {
      continue; // skip .hasm files (and anything else) — only run .vasm as
                // top-level tests
    }

    const char *name = get_filename(entry->d_name);
    printf("\n=== TEST: %s ===\n", name);

    Expected_Result *result = hashmap_get(map, name);
    if (result == NULL) {
      printf("expected result is not available for this test case (%s)\n",
             name);
      printf("skipping..\n");
      continue;
    }
    tests_run++;
    char absolute_tasm_path[PATH_MAX];
    snprintf(absolute_tasm_path, sizeof(absolute_tasm_path), "%s/%s.tasm",
             "tests/tasms", get_filename(entry->d_name));
    int program_size;
    Inst *program = deserialize_program(absolute_tasm_path, &program_size);
    bool passed = true;
    if (result->willPanic || result->isExit) {
      bool status = run_expect_panic(program, program_size, result);
      if (!status) {
        passed = false;
      }
    } else {
      if (result->isStdOut) {
        char buffer[4096] = {0};
        FILE *temp = tmpfile();
        if (!temp)
          VM_PANIC("tmpfile failed");

        fflush(stdout);
        int saved_fd = dup(fileno(stdout)); // save real stdout
        dup2(fileno(temp), fileno(stdout)); // redirect stdout -> temp

        VM vm = vm_new(program, program_size);
        vm_run(&vm); // now prints go into temp

        fflush(stdout);
        dup2(saved_fd, fileno(stdout)); // restore real stdout
        close(saved_fd);

        rewind(temp);
        fread(buffer, 1, sizeof(buffer) - 1, temp);
        fclose(temp);

        printf("expected: %s\n", result->stdOut);
        printf("received: %s\n", buffer);
        passed = (strcmp(buffer, result->stdOut) == 0);
      } else {
        VM vm = vm_new(program, program_size);
        vm_run(&vm);
        vm_dump_stack(&vm);

        if (vm.stack_pos != result->expected_sp) {
          printf("FAIL: sp mismatch (expected %d, got %d)\n",
                 result->expected_sp, vm.stack_pos);
          passed = false;
        } else if (vm.stack_pos > 0) {
          Data got = vm.stack[vm.stack_pos - 1];

          if (got.type != result->data.type) {
            printf("FAIL: type mismatch\n");
            passed = false;
          } else if (got.type == TYPE_INT) {
            printf("expected: %ld\n", result->data.word.i);
            printf("received: %ld\n", got.word.i);
            passed = (got.word.i == result->data.word.i);
          } else if (got.type == TYPE_FLOAT) {
            printf("expected: %.6f\n", result->data.word.f);
            printf("received: %.6f\n", got.word.f);
            passed = fabs(got.word.f - result->data.word.f) < 1e-9;
          } else if (got.type == TYPE_PTR) {
            printf("expected ptr: %ld\n", result->data.word.i);
            printf("received ptr: %ld\n", got.word.i);
            passed = (got.word.i == result->data.word.i);
          } else if (got.type == TYPE_STR) {
            const char *got_str = &vm.string_pool[got.word.i];
            const char *expected_str = &vm.string_pool[result->data.word.i];
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

    // Clean up allocated program memory
    free(program);
  }

  printf("\n====================\n");
  printf("Tests Run    : %d\n", tests_run);
  printf("Tests Passed : %d\n", tests_passed);
  printf("Tests Failed : %d\n", tests_run - tests_passed);

  closedir(dir);
  return (tests_passed == tests_run) ? 0 : 1;
}