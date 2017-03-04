; ModuleID = 'tests/simple_test.uai.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [5 x i8] c"%lf\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @main() {
entry:
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double 5.000000e-01)
  %1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 2)
  ret void
}
