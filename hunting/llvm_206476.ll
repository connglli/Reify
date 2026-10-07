@crc32_tab = global [256 x i32] zeroinitializer

define void @generate_crc32_table() {
  br label %1

1:                                                ; preds = %1, %0
  %2 = phi i64 [ 0, %0 ], [ %41, %1 ]
  %3 = trunc i64 %2 to i32
  %4 = and i32 %3, 1
  %5 = icmp eq i32 %4, 0
  %6 = lshr i32 %3, 5
  %7 = xor i32 %6, 249268274
  %8 = select i1 %5, i32 %6, i32 %7
  %9 = and i32 %3, 2
  %10 = icmp eq i32 %9, 0
  %11 = lshr i32 %8, 1
  %12 = xor i32 %11, 249268274
  %13 = select i1 %10, i32 %11, i32 %12
  %14 = and i32 %11, 1
  %15 = icmp eq i32 %14, 0
  %16 = and i32 %3, 16
  %17 = icmp eq i32 %16, 0
  %18 = and i32 %3, 4
  %19 = icmp eq i32 %18, 0
  %20 = lshr i32 %13, 1
  %21 = xor i32 %20, 249268274
  %22 = select i1 %19, i32 %20, i32 %21
  %23 = and i32 %3, 8
  %24 = icmp eq i32 %23, 0
  %25 = lshr i32 %22, 1
  %26 = xor i32 %25, 249268274
  %27 = select i1 %24, i32 %25, i32 %26
  %28 = xor i32 %27, 498536548
  %29 = and i32 %6, 1
  %30 = icmp eq i32 %29, 0
  %31 = select i1 %17, i32 %27, i32 %28
  %32 = xor i32 %31, 997073096
  %33 = select i1 %30, i32 %31, i32 %32
  %34 = xor i32 %33, 1994146192
  %35 = and i32 %20, 1
  %36 = icmp eq i32 %35, 0
  %37 = select i1 %15, i32 %33, i32 %34
  %38 = xor i32 %37, -306674912
  %39 = select i1 %36, i32 %37, i32 %38
  %40 = getelementptr [4 x i8], ptr @crc32_tab, i64 %2
  store i32 %39, ptr %40, align 4
  %41 = add i64 %2, 1
  %42 = icmp eq i64 %41, 256
  br i1 %42, label %43, label %1

43:                                               ; preds = %1
  ret void

; uselistorder directives
  uselistorder i32 %3, { 0, 2, 1, 4, 3, 5 }
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #0

define i32 @func_xrzgx7_60(i32 %0) {
  %2 = srem i32 %0, 46337
  %3 = mul i32 %2, 9489
  %4 = add i32 38761, %3
  %5 = urem i32 %4, 46337
  %6 = urem i32 13621, 46337
  %7 = add i32 %5, %6
  %8 = add i32 %7, -840707619
  %9 = add i32 %5, -41472
  %10 = add i32 %8, %9
  %11 = add i32 %10, %6
  %12 = add i32 %11, -89856513
  %13 = add i32 %12, %9
  %14 = add i32 %13, %6
  %15 = add i32 %14, -326663315
  %16 = add i32 %15, 13621
  %17 = add i32 %16, 1257131491
  %18 = add i32 %17, %9
  %19 = add i32 %18, 0
  %20 = add i32 %6, -13621
  %21 = add i32 %19, %20
  %22 = add i32 %21, %20
  %23 = call i32 @func_xrzgx7_69(i32 -427894912, i64 2970256246, i64 1055237019, i32 %22)
  %24 = icmp eq i32 %23, 493629208
  call void @llvm.assume(i1 %24)
  ret i32 0
}

define i32 @func_xrzgx7_69(i32 %0, i64 %1, i64 %2, i32 %3) {
  %5 = alloca [9 x i32], align 16
  store i32 %0, ptr %5, align 16
  %6 = or i32 %3, -694935483
  %7 = add i32 %6, 2085230836
  %8 = getelementptr i8, ptr %5, i64 4
  store i32 %7, ptr %8, align 4
  %9 = getelementptr i8, ptr %5, i64 8
  store i64 %1, ptr %9, align 8
  %10 = trunc i64 %2 to i32
  %11 = getelementptr i8, ptr %5, i64 16
  store i32 %10, ptr %11, align 16
  %12 = getelementptr i8, ptr %5, i64 20
  store i32 -600228105, ptr %12, align 4
  %13 = add i32 %0, -1361624650
  %14 = getelementptr i8, ptr %5, i64 28
  store i32 %13, ptr %14, align 4
  %15 = getelementptr i8, ptr %5, i64 32
  store i32 -252285656, ptr %15, align 16
  br label %16

16:                                               ; preds = %20, %4
  %17 = phi i32 [ -1, %4 ], [ %54, %20 ]
  %18 = phi i32 [ 0, %4 ], [ %55, %20 ]
  %19 = icmp slt i32 %18, 9
  br i1 %19, label %20, label %computeStatelessChecksum.exit

20:                                               ; preds = %16
  %21 = zext i32 %18 to i64
  %22 = getelementptr [4 x i8], ptr %5, i64 %21
  %23 = load i32, ptr %22, align 4
  %24 = xor i32 %23, %17
  %25 = and i32 %24, 255
  %26 = zext i32 %25 to i64
  %27 = getelementptr [4 x i8], ptr @crc32_tab, i64 %26
  %28 = load i32, ptr %27, align 4
  %29 = lshr i32 %17, 8
  %30 = xor i32 %28, %29
  %31 = lshr i32 %23, 8
  %32 = xor i32 %30, %31
  %33 = and i32 %32, 255
  %34 = zext i32 %33 to i64
  %35 = lshr i32 %30, 8
  %36 = getelementptr [4 x i8], ptr @crc32_tab, i64 %34
  %37 = load i32, ptr %36, align 4
  %38 = xor i32 %35, %37
  %39 = lshr i32 %23, 16
  %40 = xor i32 %38, %39
  %41 = and i32 %40, 255
  %42 = zext i32 %41 to i64
  %43 = lshr i32 %38, 8
  %44 = getelementptr [4 x i8], ptr @crc32_tab, i64 %42
  %45 = load i32, ptr %44, align 4
  %46 = xor i32 %43, %45
  %47 = and i32 %46, 255
  %48 = lshr i32 %23, 24
  %49 = xor i32 %47, %48
  %50 = zext i32 %49 to i64
  %51 = lshr i32 %46, 8
  %52 = getelementptr [4 x i8], ptr @crc32_tab, i64 %50
  %53 = load i32, ptr %52, align 4
  %54 = xor i32 %51, %53
  %55 = add i32 %18, 1
  br label %16

computeStatelessChecksum.exit:                    ; preds = %16
  %56 = xor i32 %17, -1
  ret i32 %56
}

define i32 @main() {
  tail call void @generate_crc32_table()
  %1 = call i32 @func_xrzgx7_60(i32 1165904883)
  ret i32 %1
}
