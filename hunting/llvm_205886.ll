target triple = "x86_64-unknown-linux-gnu"

define i32 @func_3hq21j_4(i32 %v7.0.v7.0.2979.fr.1) {
entry:
  %rem380 = srem i32 0, 46337
  %rem382 = select i1 false, i32 0, i32 %rem380
  %mul3832 = mul i32 %rem382, 0
  %rem384 = urem i32 %mul3832, 46337
  %rem380.1 = srem i32 %v7.0.v7.0.2979.fr.1, 46337
  %rem382.1 = select i1 false, i32 0, i32 %rem380.1
  %mul383.1 = mul i32 %rem382.1, 0
  %rem384.1 = urem i32 %mul383.1, 46337
  %rem380.2 = srem i32 0, 46337
  %rem382.2 = select i1 false, i32 0, i32 %rem380.2
  %mul383.2 = mul i32 %rem382.2, 0
  %rem384.2 = urem i32 %mul383.2, 46337
  %rem380.3 = srem i32 0, 46337
  %rem382.3 = select i1 false, i32 0, i32 %rem380.3
  %rem395.cmp = icmp ult i32 %rem384, 0
  %rem395.v = select i1 %rem395.cmp, i32 0, i32 0
  %0 = add i32 %mul383.2, %rem395.v
  %rem395.cmp.1 = icmp ult i32 %rem384.1, 0
  %rem395.v.1 = select i1 %rem395.cmp.1, i32 0, i32 0
  %1 = add i32 %0, %rem395.v.1
  %rem395.cmp.2 = icmp ult i32 %rem384.2, 0
  %rem395.v.2 = select i1 %rem395.cmp.2, i32 0, i32 0
  %2 = add i32 %1, %rem395.v.2
  %mul383.3 = mul i32 %rem382.3, 0
  %rem384.3 = urem i32 %mul383.3, 46337
  %rem395.cmp.3 = icmp ult i32 %rem384.3, 0
  %rem395.v.3 = select i1 %rem395.cmp.3, i32 0, i32 0
  %.neg3538 = sub i32 0, %mul383.1
  %.neg3537 = sub i32 0, %mul3832
  %3 = add i32 %2, %rem395.v.3
  %arg_13.0.lcssa = select i1 false, i32 0, i32 0
  %4 = add i32 %3, %arg_13.0.lcssa
  %5 = add i32 %.neg3537, %4
  %.neg3540 = sub i32 0, %mul383.3
  %.neg3539 = sub i32 0, %mul383.2
  %6 = add i32 %.neg3538, %5
  %7 = add i32 %.neg3539, %6
  %8 = add i32 %.neg3540, %7
  ret i32 %8
}

define i32 @main() {
entry:
  %call = call i32 @func_3hq21j_4(i32 noundef 0)
  ret i32 0
}
