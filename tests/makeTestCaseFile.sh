mkdir -p tests_vasm && cd tests_vasm

cat << 'EOF' > generate.sh
cat << 'E' > push.vasm
push 42
halt
E
cat << 'E' > add.vasm
push 10
push 20
add
halt
E
cat << 'E' > sub.vasm
push 10
push 3
sub
halt
E
cat << 'E' > mul.vasm
push 6
push 7
mul
halt
E
cat << 'E' > div.vasm
push 10
push 2
div
halt
E
cat << 'E' > mod.vasm
push 10
push 3
mod
halt
E
cat << 'E' > pop.vasm
push 1
push 2
push 3
pop
halt
E
cat << 'E' > chained.vasm
push 3
push 4
add
push 10
push 3
sub
mul
halt
E
cat << 'E' > negative_add.vasm
push -5
push 3
add
halt
E
cat << 'E' > negative_sub.vasm
push 3
push 10
sub
halt
E
cat << 'E' > int_add.vasm
push 10
push 3
add
halt
E
cat << 'E' > float_add.vasm
push_f 1.5
push_f 2.5
add
halt
E
cat << 'E' > int_mul.vasm
push 6
push 7
mul
halt
E
cat << 'E' > float_div.vasm
push_f 3.0
push_f 2.0
div
halt
E
cat << 'E' > int_mod.vasm
push 10
push 3
mod
halt
E
cat << 'E' > dup.vasm
push 5
dup
halt
E
cat << 'E' > swap.vasm
push 1
push 2
swap
halt
E
cat << 'E' > indup.vasm
push 1
push 2
push 3
indup 1
halt
E
cat << 'E' > cmp_eq_true.vasm
push 10
push 10
cmp_eq
halt
E
cat << 'E' > cmp_eq_false.vasm
push 10
push 20
cmp_eq
halt
E
cat << 'E' > cmp_lt_true.vasm
push 10
push 20
cmp_lt
halt
E
cat << 'E' > cmp_gt_false.vasm
push 10
push 20
cmp_gt
halt
E
cat << 'E' > cmp_eq_float.vasm
push_f 1.5
push_f 1.5
cmp_eq
halt
E
cat << 'E' > jmp.vasm
push 1
jmp 4
push 99
halt
halt
E
cat << 'E' > zjmp_taken.vasm
push 0
zjmp 4
push 1
halt
halt
E
cat << 'E' > zjmp_not_taken.vasm
push 1
zjmp 4
push 99
halt
halt
E
cat << 'E' > square_call.vasm
push 4
call 3
halt
dup
mul
ret
E
cat << 'E' > fib_5.vasm
push 5
push 2
push 0
push 1
indup 1
indup 1
add
indup 3
push 1
add
indup 5
sub
zjmp 22
indup 3
push 1
add
inswap 4
pop
swap
inswap 2
pop
jmp 4
halt
E
cat << 'E' > mov_imm.vasm
mov_imm 0, 42
push_reg 0
halt
E
cat << 'E' > mov_imm_float.vasm
mov_imm_f 0, 3.5
push_reg 0
halt
E
cat << 'E' > mov_top.vasm
push 10
mov_top 1
push_reg 1
halt
E
cat << 'E' > register_add.vasm
mov_imm 0, 1
mov_imm 1, 2
push_reg 0
push_reg 1
add
halt
E
cat << 'E' > register_index_5.vasm
mov_imm 5, 99
push_reg 5
halt
E
cat << 'E' > alloc.vasm
push 4
alloc
halt
E
cat << 'E' > write_then_read.vasm
push 4
alloc
dup
push 42
swap
write
read
halt
E
cat << 'E' > alloc_then_dealloc.vasm
push 4
alloc
dealloc
halt
E
cat << 'E' > two_independent_blocks.vasm
push 2
alloc
dup
push 11
swap
write
push 2
alloc
dup
push 22
swap
write
read
swap
read
halt
E
cat << 'E' > invalid_register.vasm
mov_imm 16, 2
halt
E
cat << 'E' > double_dealloc.vasm
push 4
alloc
dup
dealloc
dealloc
halt
E
cat << 'E' > read_with_non-pointer_type.vasm
push 123456
read
halt
E
cat << 'E' > write_with_non-pointer_type.vasm
push 123456
push 42
write
halt
E
cat << 'E' > alloc_negative_size.vasm
push -1
alloc
halt
E
cat << 'E' > alloc_zero_size.vasm
push 0
alloc
halt
E
cat << 'E' > push_str_hi.vasm
push_str "hi"
halt
E
cat << 'E' > push_str_two.vasm
push_str "hello"
push_str "world"
halt
E
cat << 'E' > push_str_empty.vasm
push_str ""
halt
E
cat << 'E' > itof_positive.vasm
push 5
itof
halt
E
cat << 'E' > ftoi_positive_truncate.vasm
push_f 3.7
ftoi
halt
E
cat << 'E' > ftoi_negative_truncate.vasm
push_f -3.7
ftoi
halt
E
cat << 'E' > itoc_valid_range.vasm
push 65
itoc
halt
E
cat << 'E' > itoc_overflow_panic.vasm
push 256
itoc
halt
E
cat << 'E' > itoc_underflow_panic.vasm
push -1
itoc
halt
E
cat << 'E' > toi_int_pass-through.vasm
push 5
toi
halt
E
cat << 'E' > tof_float_pass-through.vasm
push_f 5.0
tof
halt
E
cat << 'E' > toi_float_to_int.vasm
push_f 9.9
toi
halt
E
cat << 'E' > tof_int_to_float.vasm
push 9
tof
halt
E
cat << 'E' > print_int.vasm
push 42
native print_int
halt
E
cat << 'E' > print_float.vasm
push_f 4.24
native print_float
halt
E
cat << 'E' > print_char.vasm
push 66
native print_char
halt
E
cat << 'E' > print_str.vasm
push_str "hello world!"
native print_str
halt
E
cat << 'E' > exit_vm.vasm
push 0
native exit_vm
E
cat << 'E' > common.hasm
@def STDOUT 1
E
cat << 'E' > main.vasm
@imp "common.hasm"
push 42
push STDOUT
native print_int
halt
E
cat << 'E' > type_error_in_call.vasm
@imp "common.hasm"
call 3
halt
push_f 1.5
push 10
add
ret
E
cat << 'E' > stack_overflow.vasm
call 0
halt
E
cat << 'E' > div_zero_in_call.vasm
call 3
halt
push 10
push 0
div
ret
E
cat << 'E' > type_mismatch_nested.vasm
call 2
halt
call 5
ret
push_f 1.5
push 10
mul
ret
E
cat << 'E' > load_fn_test.vasm
load_lib "mylib.so"
load_fn "my_double"
push 21
call_native
native print_int
halt
E
cat << 'E' > fizzbuzz.vasm
push 20
push 1
dup
indup 2
cmp_gt
nzjmp 36
dup 
push 15
mod
 zjmp 21
  dup
  push 3
  mod
  zjmp 24
  dup
 push 5
 mod
 zjmp 27
 dup
 native print_int
 jmp 30
  push_str "FizzBuzz"
 native print_str
 jmp 30
  push_str "Fizz"
  native print_str
  jmp 30
 push_str "Buzz"
  native print_str
 jmp 30
  dup
 push 1
 add
 swap
 pop
jmp 2
halt
E
rm generate.sh
EOF
bash generate.sh