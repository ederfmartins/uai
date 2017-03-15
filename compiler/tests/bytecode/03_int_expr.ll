; ModuleID = '/home/ederfm/work/uai/compiler/tests/03_int_expr.bc'

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
  %.tmp23 = load i32, i32* %a
  %.tmp24 = load i32, i32* %b
  %.tmp25 = icmp sgt i32 %.tmp23, %.tmp24
  %.print26 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp25)
  %.tmp27 = load i32, i32* %b
  %.tmp28 = load i32, i32* %a
  %.tmp29 = icmp sgt i32 %.tmp27, %.tmp28
  %.print30 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp29)
  %.tmp31 = load i32, i32* %b
  %.tmp32 = load i32, i32* %a
  %.tmp33 = icmp slt i32 %.tmp31, %.tmp32
  %.print34 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp33)
  %.tmp35 = load i32, i32* %a
  %.tmp36 = load i32, i32* %b
  %.tmp37 = icmp slt i32 %.tmp35, %.tmp36
  %.print38 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp37)
  %.tmp39 = load i32, i32* %a
  %.tmp40 = load i32, i32* %b
  %.tmp41 = icmp sge i32 %.tmp39, %.tmp40
  %.print42 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp41)
  %.tmp43 = load i32, i32* %a
  %.tmp44 = icmp sge i32 %.tmp43, 10
  %.print45 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp44)
  %.tmp46 = load i32, i32* %b
  %.tmp47 = load i32, i32* %a
  %.tmp48 = icmp sge i32 %.tmp46, %.tmp47
  %.print49 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp48)
  %.tmp50 = load i32, i32* %b
  %.tmp51 = load i32, i32* %a
  %.tmp52 = icmp sle i32 %.tmp50, %.tmp51
  %.print53 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp52)
  %.tmp54 = load i32, i32* %a
  %.tmp55 = load i32, i32* %b
  %.tmp56 = icmp sle i32 %.tmp54, %.tmp55
  %.print57 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp56)
  %.tmp58 = load i32, i32* %a
  %.tmp59 = icmp sle i32 %.tmp58, 10
  %.print60 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp59)
  %.tmp61 = load i32, i32* %a
  %.tmp62 = load i32, i32* %b
  %.tmp63 = icmp eq i32 %.tmp61, %.tmp62
  %.print64 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp63)
  %.tmp65 = load i32, i32* %a
  %.tmp66 = load i32, i32* %b
  %.tmp67 = icmp ne i32 %.tmp65, %.tmp66
  %.print68 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp67)
  %.tmp69 = load i32, i32* %a
  %.tmp70 = icmp eq i32 %.tmp69, 10
  %.print71 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp70)
  %.tmp72 = load i32, i32* %a
  %.tmp73 = icmp ne i32 %.tmp72, 10
  %.print74 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i1 %.tmp73)
  br label %end

end:                                              ; preds = %entry
  ret void
}
