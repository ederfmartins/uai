; ModuleID = '/home/ederfm/work/uai/compiler/tests/02_sum_constants.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [5 x i8] c"%lf\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @main() {
entry:
  ret void
}
