#ifndef SERIALIZE_H
#define SERIALIZE_H
#include "vm.h"

bool serialize_program(const Inst *program, size_t count, const char *filepath);
Inst *deserialize_program(const char *filepath, int *out_count);
#endif