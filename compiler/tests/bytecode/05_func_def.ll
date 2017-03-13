; ModuleID = '/home/ederfm/work/uai/compiler/tests/05_func_def.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [5 x i8] c"%lf\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @void_x() {
entry:
  ret void
}

define i32 @plus_2(i32) {
entry:
  %.param = alloca i32
  store i32 %0, i32* %.param
  %.tmp = load i32, i32* %.param
  %.tmp1 = add i32 %.tmp, 2
  ret i32 %.tmp1
}

define void @func(i32, double, i1, i32) {
entry:
  %.param = alloca i32
  store i32 %0, i32* %.param
  %.param1 = alloca double
  store double %1, double* %.param1
  %.param2 = alloca i1
  store i1 %2, i1* %.param2
  %.param3 = alloca i32
  store i32 %3, i32* %.param3
  %.tmp = load i32, i32* %.param
  %.print = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp)
  %.tmp4 = load double, double* %.param1
  %.print5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.tmp4)
  %.tmp6 = load i1, i1* %.param2
  %.print7 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp6)
  %.tmp8 = load i32, i32* %.param3
  %.print9 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp8)
  ret void
}

define double @ret_42() {
entry:
  ret double 4.200000e+01
}

define void @main() {
entry:
  %x = alloca i32
  store i32 1, i32* %x
  %.tmp = load i32, i32* %x
  %.ret = call i32 @plus_2(i32 %.tmp)
  %y = alloca i32
  store i32 %.ret, i32* %y
  %.tmp1 = load i32, i32* %y
  %.print = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.tmp1)
  call void @void_x()
  %.tmp2 = load i32, i32* %x
  %.tmp3 = load i32, i32* %y
  call void @func(i32 %.tmp2, double 1.450000e+00, i1 false, i32 %.tmp3)
  %.ret4 = call double @ret_42()
  %.print5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.ret4)
  ret void
}
