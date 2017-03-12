; ModuleID = '/home/ederfm/work/uai/compiler/tests/int_expr.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [5 x i8] c"%lf\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @main() {
entry:
  %a = alloca i32
  store i32 10, i32* %a
  %.tmp = load i32, i32* %a
  %.tmp1 = mul i32 2, %.tmp
  %.tmp2 = add i32 3, %.tmp1
  %b = alloca i32
  store i32 %.tmp2, i32* %b
  %.tmp3 = load i32, i32* %b
  %.print = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp3)
  %.tmp4 = load i32, i32* %b
  %.tmp5 = sub i32 %.tmp4, 5
  %.tmp6 = load i32, i32* %a
  %.tmp7 = sub i32 15, %.tmp6
  %.tmp8 = mul i32 %.tmp5, %.tmp7
  %c = alloca i32
  store i32 %.tmp8, i32* %c
  %.tmp9 = load i32, i32* %c
  %.print10 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp9)
  %.tmp11 = load i32, i32* %a
  %.tmp12 = load i32, i32* %b
  %.tmp13 = mul i32 %.tmp11, %.tmp12
  %.print14 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp13)
  %.tmp15 = load i32, i32* %b
  %.tmp16 = load i32, i32* %a
  %.tmp17 = sdiv i32 %.tmp15, %.tmp16
  %.print18 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp17)
  %.tmp19 = load i32, i32* %b
  %.tmp20 = load i32, i32* %a
  %.tmp21 = srem i32 %.tmp19, %.tmp20
  %.print22 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp21)
  ret void
}
