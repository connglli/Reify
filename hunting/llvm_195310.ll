target triple = "x86_64-unknown-linux-gnu"

@arr= global [256 x i32] zeroinitializer

define i32 @main(i32 %0) {
entry:
  %__chk_args.i.i = alloca [6 x i32], align 16
  br label %vector.body.i

vector.body.i:                                    ; preds = %vector.body.i, %entry
  %index.i = phi i64 [ 0, %entry ], [ %index.next.i, %vector.body.i ]
  %vec.ind.i = phi <4 x i32> [ zeroinitializer, %entry ], [ %vec.ind.next.i, %vector.body.i ]
  %1 = and <4 x i32> %vec.ind.i, splat (i32 4)
  %2 = icmp eq <4 x i32> %1, zeroinitializer
  %3 = select <4 x i1> %2, <4 x i32> <i32 0, i32 124634137, i32 249268274, i32 162941995>, <4 x i32> <i32 498536548, i32 450548861, i32 325883990, i32 335633487>
  %4 = lshr <4 x i32> %vec.ind.i, splat (i32 6)
  %5 = xor <4 x i32> %4, <i32 498536548, i32 450548861, i32 325883990, i32 335633487>
  %6 = and <4 x i32> %5, splat (i32 1)
  %7 = icmp eq <4 x i32> %6, zeroinitializer
  %8 = and <4 x i32> %vec.ind.i, splat (i32 16)
  %9 = icmp eq <4 x i32> %8, zeroinitializer
  %10 = and <4 x i32> %vec.ind.i, splat (i32 8)
  %11 = icmp eq <4 x i32> %10, zeroinitializer
  %12 = lshr <4 x i32> %3, splat (i32 2)
  %13 = xor <4 x i32> %12, splat (i32 249268274)
  %14 = select <4 x i1> %11, <4 x i32> %12, <4 x i32> %13
  %15 = xor <4 x i32> %14, splat (i32 498536548)
  %16 = and <4 x i32> %vec.ind.i, splat (i32 32)
  %17 = icmp eq <4 x i32> %16, zeroinitializer
  %18 = select <4 x i1> %9, <4 x i32> %14, <4 x i32> %15
  %19 = xor <4 x i32> %18, splat (i32 997073096)
  %20 = select <4 x i1> %17, <4 x i32> %18, <4 x i32> %19
  %21 = xor <4 x i32> %20, splat (i32 1994146192)
  %22 = and <4 x i32> %5, splat (i32 2)
  %23 = icmp eq <4 x i32> %22, zeroinitializer
  %24 = select <4 x i1> %7, <4 x i32> %20, <4 x i32> %21
  %25 = xor <4 x i32> %24, splat (i32 -306674912)
  %26 = select <4 x i1> %23, <4 x i32> %24, <4 x i32> %25
  %27 = getelementptr [4 x i8], ptr @arr, i64 %index.i
  store <4 x i32> %26, ptr %27, align 16
  %index.next.i = add i64 %index.i, 4
  %vec.ind.next.i = add <4 x i32> %vec.ind.i, splat (i32 4)
  %28 = icmp eq i64 %index.next.i, 256
  br i1 %28, label %generate_arr.exit, label %vector.body.i

generate_arr.exit:                        ; preds = %vector.body.i
  %rem1064.i = urem i32 %0, 46337
  %29 = insertelement <2 x i32> splat (i32 1), i32 %rem1064.i, i64 1
  %30 = shufflevector <2 x i32> %29, <2 x i32> zeroinitializer, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  %31 = insertelement <2 x i32> %29, i32 1, i64 1
  %32 = shufflevector <2 x i32> %31, <2 x i32> zeroinitializer, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  %33 = mul <4 x i32> %30, <i32 16313, i32 14818, i32 26897, i32 21566>
  %34 = urem <4 x i32> %33, splat (i32 46337)
  %35 = mul <4 x i32> %32, %34
  %.fr.i = freeze <4 x i32> %35
  store i32 -3, ptr %__chk_args.i.i, align 16
  %arrayinit.element.i.i = getelementptr i8, ptr %__chk_args.i.i, i64 4
  store i32 1, ptr %arrayinit.element.i.i, align 4
  %36 = extractelement <4 x i32> %.fr.i, i64 2
  %37 = add i32 %36, 38880
  %38 = urem i32 %37, 46337
  %add360.i.i = sub i32 19435, %38
  %arrayinit.element365.i.i = getelementptr i8, ptr %__chk_args.i.i, i64 8
  store i32 %add360.i.i, ptr %arrayinit.element365.i.i, align 8
  %arrayinit.element366.i.i = getelementptr i8, ptr %__chk_args.i.i, i64 12
  store i32 -1, ptr %arrayinit.element366.i.i, align 4
  %arrayinit.element367.i.i = getelementptr i8, ptr %__chk_args.i.i, i64 16
  store i32 2, ptr %arrayinit.element367.i.i, align 16
  %arrayinit.element368.i.i = getelementptr i8, ptr %__chk_args.i.i, i64 20
  store i32 1, ptr %arrayinit.element368.i.i, align 4
  br label %for.body.i.i.i

for.body.i.i.i:                                   ; preds = %for.body.i.i.i, %generate_arr.exit
  %indvars.iv.i.i.i = phi i64 [ 0, %generate_arr.exit ], [ %indvars.iv.next.i.i.i, %for.body.i.i.i ]
  %checksum.08.i.i.i = phi i32 [ -1, %generate_arr.exit ], [ %xor2.i37.i.i.i.i, %for.body.i.i.i ]
  %arrayidx.i.i.i = getelementptr [4 x i8], ptr %__chk_args.i.i, i64 %indvars.iv.i.i.i
  %39 = load i32, ptr %arrayidx.i.i.i, align 4
  %xor.narrow.i38.i.i.i.i = xor i32 %39, %checksum.08.i.i.i
  %40 = and i32 %xor.narrow.i38.i.i.i.i, 255
  %idxprom.i.i.i.i.i = zext i32 %40 to i64
  %arrayidx.i.i.i.i.i = getelementptr [4 x i8], ptr @arr, i64 %idxprom.i.i.i.i.i
  %41 = load i32, ptr %arrayidx.i.i.i.i.i, align 4
  %shr.i.i.i.i.i = lshr i32 %checksum.08.i.i.i, 8
  %xor2.i.i.i.i.i = xor i32 %41, %shr.i.i.i.i.i
  %shr1.i.i.i.i = lshr i32 %39, 8
  %xor.narrow.i2239.i.i.i.i = xor i32 %xor2.i.i.i.i.i, %shr1.i.i.i.i
  %42 = and i32 %xor.narrow.i2239.i.i.i.i, 255
  %idxprom.i23.i.i.i.i = zext i32 %42 to i64
  %shr.i20.i.i.i.i = lshr i32 %xor2.i.i.i.i.i, 8
  %arrayidx.i24.i.i.i.i = getelementptr [4 x i8], ptr @arr, i64 %idxprom.i23.i.i.i.i
  %43 = load i32, ptr %arrayidx.i24.i.i.i.i, align 4
  %xor2.i25.i.i.i.i = xor i32 %shr.i20.i.i.i.i, %43
  %shr5.i.i.i.i = lshr i32 %39, 16
  %xor.narrow.i2840.i.i.i.i = xor i32 %xor2.i25.i.i.i.i, %shr5.i.i.i.i
  %44 = and i32 %xor.narrow.i2840.i.i.i.i, 255
  %idxprom.i29.i.i.i.i = zext i32 %44 to i64
  %shr.i26.i.i.i.i = lshr i32 %xor2.i25.i.i.i.i, 8
  %arrayidx.i30.i.i.i.i = getelementptr [4 x i8], ptr @arr, i64 %idxprom.i29.i.i.i.i
  %45 = load i32, ptr %arrayidx.i30.i.i.i.i, align 4
  %xor2.i31.i.i.i.i = xor i32 %shr.i26.i.i.i.i, %45
  %xor2.i31.masked.i.i.i.i = and i32 %xor2.i31.i.i.i.i, 255
  %shr9.i.i.i.i = lshr i32 %39, 24
  %46 = xor i32 %xor2.i31.masked.i.i.i.i, %shr9.i.i.i.i
  %idxprom.i35.i.i.i.i = zext i32 %46 to i64
  %shr.i32.i.i.i.i = lshr i32 %xor2.i31.i.i.i.i, 8
  %arrayidx.i36.i.i.i.i = getelementptr [4 x i8], ptr @arr, i64 %idxprom.i35.i.i.i.i
  %47 = load i32, ptr %arrayidx.i36.i.i.i.i, align 4
  %xor2.i37.i.i.i.i = xor i32 %shr.i32.i.i.i.i, %47
  %indvars.iv.next.i.i.i = add i64 %indvars.iv.i.i.i, 1
  %exitcond.not.i.i.i = icmp eq i64 %indvars.iv.next.i.i.i, 6
  br i1 %exitcond.not.i.i.i, label %func_m9fqmi_1.exit.i, label %for.body.i.i.i

func_m9fqmi_1.exit.i:                             ; preds = %for.body.i.i.i
  %48 = xor i32 %xor2.i37.i.i.i.i, -1
  %49 = urem i32 %48, 2147483647
  %cmp.not.i1281.i.i = icmp eq i32 %49, 2053482772
  br i1 %cmp.not.i1281.i.i, label %func_m9fqmi_58.exit, label %if.then.i1282.i.i

if.then.i1282.i.i:                                ; preds = %func_m9fqmi_1.exit.i
  ret i32 1
  unreachable

func_m9fqmi_58.exit:                              ; preds = %func_m9fqmi_1.exit.i
  ret i32 0
}
