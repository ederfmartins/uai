; ModuleID = 'tests/uai/02_sum_constants.uai.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [4 x i8] c"%f\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @main() {
entry:
  ret void
}
