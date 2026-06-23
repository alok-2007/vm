#include "./src/serialize.h"
#include "./src/vasm.h"
#include "./src/vm.h"

Inst *assemble(char *sourceFile, size_t *outSize) {
  const char *rawSrc = read_source_from_disk(sourceFile);
  HashMap *def_Map = hashmap_new(100);
  int stack_capacity = 16;
  const char **import_stack = malloc(stack_capacity * sizeof(char *));
  import_stack[0] = mystrdup(sourceFile);
  const char *preProcessed =
      preprocessor(rawSrc, "", &import_stack, 1, &stack_capacity, def_Map);
  TokenList tokens = lex(preProcessed);
  Inst *program = parse(tokens, outSize);
  return program;
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    char *sourceFile = argv[1];
    size_t program_size;
    Inst *program = assemble(sourceFile, &program_size);
    VM vm = vm_new(program, program_size);
    vm_run(&vm);
    vm_dump_stack(&vm);
  } else if (argc == 3 && strcmp(argv[1], "--run") == 0) {
    char *bytecodeFile = argv[2];
    int program_size = 0;
    Inst *program = deserialize_program(bytecodeFile, &program_size);
    VM vm = vm_new(program, program_size);
    vm_run(&vm);
    vm_dump_stack(&vm);
  } else if (argc == 5 && strcmp(argv[1], "--compile") == 0 &&
             strcmp(argv[3], "-o") == 0) {
    char *sourceFile = argv[2];
    size_t program_size = 0;
    Inst *program = assemble(sourceFile, &program_size);
    char *targetFile = argv[4];
    bool result = serialize_program(program, program_size, targetFile);
    if (!result) {
      VM_PANIC("error while compiling");
    }
  } else {
    VM_PANIC(
        "Usage:\n\t./vm <file.vasm>\t\t (FILE : SOURCE)\n\t./vm --run "
        "<file.vbc>\t\t (FILE: BYTECODE)\n\t./vm --compile <file.vasm> -o "
        "<file.vbc>\t\t (FILE: <.vasm> source code, <.vbc>  target bytecode)");
  }
  return 0;
}