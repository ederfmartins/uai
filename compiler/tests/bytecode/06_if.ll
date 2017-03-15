; ModuleID = 'tests/uai/06_if.uai.bc'

@.print_arg_d = private unnamed_addr constant [4 x i8] c"%d\0A\00"
@.print_arg_f = private unnamed_addr constant [5 x i8] c"%lf\0A\00"

declare i32 @printf(i8* noalias, ...)

define i32 @if_else_const(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  store i32 1, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  store i32 0, i32* %.ret
  br label %end

end:                                              ; preds = %.iffalse, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @if_else_1(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  %.tmp2 = load i32, i32* %.p
  store i32 %.tmp2, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  %.tmp3 = load i32, i32* %.p
  %.tmp4 = add i32 %.tmp3, 1
  store i32 %.tmp4, i32* %.ret
  br label %end

end:                                              ; preds = %.iffalse, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @if_else_2(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  %.tmp2 = load i32, i32* %.p
  store i32 %.tmp2, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  store i32 1, i32* %.ret
  br label %end

end:                                              ; preds = %.iffalse, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @if_else_3(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  store i32 1, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  %.tmp2 = load i32, i32* %.p
  store i32 %.tmp2, i32* %.ret
  br label %end

end:                                              ; preds = %.iffalse, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @only_if(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  store i32 1, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  br label %.endif

.endif:                                           ; preds = %.iffalse
  store i32 -1, i32* %.ret
  br label %end

end:                                              ; preds = %.endif, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @only_if1(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  store i32 1, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  br label %.endif

.endif:                                           ; preds = %.iffalse
  %.tmp2 = load i32, i32* %.p
  store i32 %.tmp2, i32* %.ret
  br label %end

end:                                              ; preds = %.endif, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @only_if2(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  %.tmp2 = load i32, i32* %.p
  store i32 %.tmp2, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  br label %.endif

.endif:                                           ; preds = %.iffalse
  store i32 0, i32* %.ret
  br label %end

end:                                              ; preds = %.endif, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define i32 @only_if3(i32 %x) {
entry:
  %.ret = alloca i32
  %.p = alloca i32
  store i32 %x, i32* %.p
  %.tmp = load i32, i32* %.p
  %.tmp1 = icmp sgt i32 %.tmp, 0
  br i1 %.tmp1, label %.iftrue, label %.iffalse

.iftrue:                                          ; preds = %entry
  store i32 1, i32* %.ret
  br label %end

.iffalse:                                         ; preds = %entry
  br label %.endif

.endif:                                           ; preds = %.iffalse
  %.tmp2 = load i32, i32* %.p
  %.tmp3 = add i32 %.tmp2, 2
  store i32 %.tmp3, i32* %.ret
  br label %end

end:                                              ; preds = %.endif, %.iftrue
  %.ret_reg = load i32, i32* %.ret
  ret i32 %.ret_reg
}

define void @main() {
entry:
  %.ret = call i32 @if_else_const(i32 1)
  %.print = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret)
  %.ret1 = call i32 @if_else_const(i32 -1)
  %.print2 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret1)
  %.ret3 = call i32 @if_else_1(i32 1)
  %.print4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret3)
  %.ret5 = call i32 @if_else_1(i32 -1)
  %.print6 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret5)
  %.ret7 = call i32 @if_else_2(i32 1)
  %.print8 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret7)
  %.ret9 = call i32 @if_else_2(i32 -1)
  %.print10 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret9)
  %.ret11 = call i32 @if_else_3(i32 1)
  %.print12 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret11)
  %.ret13 = call i32 @if_else_3(i32 -1)
  %.print14 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret13)
  %.ret15 = call i32 @only_if(i32 1)
  %.print16 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret15)
  %.ret17 = call i32 @only_if(i32 -1)
  %.print18 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret17)
  %.ret19 = call i32 @only_if1(i32 1)
  %.print20 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret19)
  %.ret21 = call i32 @only_if1(i32 -1)
  %.print22 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret21)
  %.ret23 = call i32 @only_if2(i32 1)
  %.print24 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret23)
  %.ret25 = call i32 @only_if2(i32 -1)
  %.print26 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret25)
  %.ret27 = call i32 @only_if3(i32 1)
  %.print28 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret27)
  %.ret29 = call i32 @only_if3(i32 -1)
  %.print30 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.print_arg_d, i32 0, i32 0), i32 %.ret29)
  br label %end

end:                                              ; preds = %entry
  ret void
}
