target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memmove.p0.p0.i64(ptr writeonly captures(none), ptr readonly captures(none), i64, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #1

define fastcc i32 @func_nno0tw_28(ptr %v1) {
for_body_751.preheader:
  %aliasCheck_50_0.sroa.0 = alloca [44 x i32], align 16
  %arrayidx107 = getelementptr i8, ptr %v1, i64 4
  %arrayidx244 = getelementptr i8, ptr %v1, i64 8
  %0 = load i32, ptr %arrayidx244, align 4
  %rem2916 = srem i32 %0, 46337
  %add2917 = add i32 %rem2916, 46337
  %mul2919 = mul i32 %add2917, 16263
  %add2923 = add i32 330, %mul2919
  %1 = load i32, ptr %arrayidx107, align 4
  %rem2830 = srem i32 %1, 46337
  %aliasCheck_50_0.sroa.0.44.arrayidx3011.sroa_idx3 = getelementptr i8, ptr %aliasCheck_50_0.sroa.0, i64 44
  store i32 -2116438905, ptr %aliasCheck_50_0.sroa.0.44.arrayidx3011.sroa_idx3, align 4
  %aliasCheck_50_0.sroa.0.48.arrayidx3012.sroa_idx4 = getelementptr i8, ptr %aliasCheck_50_0.sroa.0, i64 48
  store i32 1919032160, ptr %aliasCheck_50_0.sroa.0.48.arrayidx3012.sroa_idx4, align 16
  %aliasCheck_50_0.sroa.0.44.arrayidx3011.sroa_idx2 = getelementptr i8, ptr %aliasCheck_50_0.sroa.0, i64 44
  call void @llvm.memmove.p0.p0.i64(ptr %aliasCheck_50_0.sroa.0, ptr %aliasCheck_50_0.sroa.0.44.arrayidx3011.sroa_idx2, i64 132, i1 false)
  %mul2833 = mul i32 %rem2830, 3724
  %mul2840 = mul i32 %rem2830, 16306
  %add2842 = add i32 %mul2833, %mul2840
  %add2844 = add i32 -16457, %add2842
  %reass.sub = sub i32 0, %add2844
  %add3182.us.2.4 = add i32 %reass.sub, 8113750
  %add3178 = add i32 0, 0
  %rem2924 = urem i32 %add2923, 46337
  %add2925 = add i32 %rem2924, 64148112
  %add3182.us.2.3 = add i32 %add3178, %add2925
  %reass.add1498 = shl i32 %add3178, 0
  %add3230.us.1.3 = add i32 0, %reass.add1498
  %add3230.us.2.3 = add i32 %add3182.us.2.3, %add3230.us.1.3
  %reass.add1499 = shl i32 %add3178, 0
  %add3230.us.1.4 = add i32 %add3230.us.2.3, %reass.add1499
  %add3182.us.2.5 = add i32 %add3178, 79842678
  %add3230.us.2.4 = add i32 %add3182.us.2.4, %add3230.us.1.4
  %reass.add1500 = shl i32 %add3178, 0
  %add3230.us.1.5 = add i32 %add3230.us.2.4, %reass.add1500
  %add3230.us.2.5 = add i32 %add3182.us.2.5, %add3230.us.1.5
  %reass.add1501 = shl i32 %add3178, 0
  %aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0. = load i32, ptr %aliasCheck_50_0.sroa.0, align 16
  %aliasCheck_50_0.sroa.0.4.arrayidx3073.sroa_idx1 = getelementptr i8, ptr %aliasCheck_50_0.sroa.0, i64 4
  %aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4. = load i32, ptr %aliasCheck_50_0.sroa.0.4.arrayidx3073.sroa_idx1, align 4
  %add3077 = add i32 %aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0.aliasCheck_50_0.sroa.0.0., %aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.aliasCheck_50_0.sroa.0.4.
  %add3182.us.2.7 = add i32 %add3178, %add3077
  %add3182.us.2.6 = add i32 %add3178, 45324178
  %add3230.us.1.6 = add i32 %add3230.us.2.5, %reass.add1501
  %add3230.us.2.6 = add i32 %add3182.us.2.6, %add3230.us.1.6
  %reass.add1502 = shl i32 %add3178, 1
  %add3230.us.1.7 = add i32 %add3230.us.2.6, %reass.add1502
  %add3182.us.2.9 = add i32 %add3178, 1339756373
  %add3182.us.2.8 = add i32 %add3178, -588265445
  %add3230.us.2.7 = add i32 %add3182.us.2.7, %add3230.us.1.7
  %reass.add1503 = shl i32 %add3178, 1
  %add3230.us.1.8 = add i32 %add3230.us.2.7, %reass.add1503
  %add3230.us.2.8 = add i32 %add3182.us.2.8, %add3230.us.1.8
  %reass.add1504 = shl i32 %add3178, 1
  %add3230.us.1.9 = add i32 %add3230.us.2.8, %reass.add1504
  %add3182.us.2.10 = add i32 %add3178, 389369760
  %add3230.us.2.9 = add i32 %add3182.us.2.9, %add3230.us.1.9
  %reass.add1505 = shl i32 %add3178, 1
  %add3230.us.1.10 = add i32 %add3230.us.2.9, %reass.add1505
  %add3230.us.2.10 = add i32 %add3182.us.2.10, %add3230.us.1.10
  %rem.i = srem i32 %add3230.us.2.10, 46337
  %mul10.i = mul i32 %rem.i, 21578
  %rem11.i = urem i32 %mul10.i, 46337
  %mul15.i = mul i32 2676, %rem11.i
  %rem16.i = urem i32 %mul15.i, 46337
  %add23.i = add i32 %rem16.i, 14604
  %and.i = and i32 %add23.i, 61889
  %add27.i = add i32 %and.i, 1748154851
  %or.i = or i32 %add23.i, 89846209
  %add30.i = add i32 %add27.i, %or.i
  %sub1.i.i = sub i32 1935415880, %add30.i
  %rem.i.i = srem i32 -659453650, %sub1.i.i
  %reass.sub.i.i = sub i32 %rem.i.i, %add30.i
  %cmp.i.i = icmp eq i32 %reass.sub.i.i, -1913265558
  call void @llvm.assume(i1 %cmp.i.i)
  ret i32 0

; uselistorder directives
  uselistorder i32 %add3178, { 0, 1, 2, 5, 3, 4, 6, 14, 7, 8, 9, 10, 11, 12, 13 }
}

define i32 @main() {
entry:
  %.compoundliteral580.i = alloca [3 x [2 x i32]], align 16
  store <4 x i32> <i32 1010353832, i32 1547967315, i32 -1059885678, i32 0>, ptr %.compoundliteral580.i, align 16
  %call592.i = call fastcc i32 @func_nno0tw_28(ptr %.compoundliteral580.i)
  ret i32 %call592.i
}

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
