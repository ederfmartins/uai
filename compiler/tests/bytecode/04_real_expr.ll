; ModuleID = '/home/ederfm/work/uai/compiler/tests/04_real_expr.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [5 x i8] c"%lf\0A\00"

declare i32 @printf(i8* noalias, ...)

define void @main() {
entry:
  %a = alloca double
  store double 1.000000e+01, double* %a
  %.tmp = load double, double* %a
  %.tmp1 = fmul double 2.000000e+00, %.tmp
  %.tmp2 = fadd double 3.000000e+00, %.tmp1
  %b = alloca double
  store double %.tmp2, double* %b
  %.tmp3 = load double, double* %b
  %.print = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.tmp3)
  %.tmp4 = load double, double* %b
  %.tmp5 = fsub double %.tmp4, 5.000000e+00
  %.tmp6 = load double, double* %a
  %.tmp7 = fsub double 1.500000e+01, %.tmp6
  %.tmp8 = fmul double %.tmp5, %.tmp7
  %c = alloca double
  store double %.tmp8, double* %c
  %.tmp9 = load double, double* %c
  %.print10 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.tmp9)
  %.tmp11 = load double, double* %a
  %.tmp12 = load double, double* %b
  %.tmp13 = fmul double %.tmp11, %.tmp12
  %.print14 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.tmp13)
  %.tmp15 = load double, double* %b
  %.tmp16 = load double, double* %a
  %.tmp17 = fdiv double %.tmp15, %.tmp16
  %.print18 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.tmp17)
  %.tmp19 = load double, double* %b
  %.tmp20 = load double, double* %a
  %.tmp21 = frem double %.tmp19, %.tmp20
  %.print22 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.print_arg_f, i32 0, i32 0), double %.tmp21)
  ret void
}
