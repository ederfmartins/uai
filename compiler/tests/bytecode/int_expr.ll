; ModuleID = 'tests/simple_test.uai.bc'

@.print_arg = private unnamed_addr constant [4 x i8] c"%d\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @main() {
entry:
  %0 = alloca i32
  store i32 10, i32* %0
  %1 = load i32, i32* %0
  %2 = mul i32 2, %1
  %3 = add i32 3, %2
  %4 = alloca i32
  store i32 %3, i32* %4
  %5 = load i32, i32* %4
  %6 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg, i32 0, i32 0), i32 %5)
  %7 = load i32, i32* %4
  %8 = sub i32 %7, 5
  %9 = alloca i32
  store i32 %8, i32* %9
  %10 = load i32, i32* %9
  %11 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg, i32 0, i32 0), i32 %10)
  %12 = load i32, i32* %0
  %13 = load i32, i32* %4
  %14 = mul i32 %12, %13
  %15 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg, i32 0, i32 0), i32 %14)
  ret void
}
