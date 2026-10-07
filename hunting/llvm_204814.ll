; ModuleID = '<bc file>'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main(i32 %v8.addr.0.v8.addr.0.2281.fr.i.1) {
entry:
  %rem943.i = srem i32 0, 46337
  %rem945.cmp.i = icmp slt i32 %rem943.i, 0
  %rem945.i = select i1 %rem945.cmp.i, i32 0, i32 0
  %mul946.i = mul i32 %rem945.i, 0
  %rem947.i = urem i32 %mul946.i, 46337
  %rem943.i.1 = srem i32 %v8.addr.0.v8.addr.0.2281.fr.i.1, 46337
  %rem945.cmp.i.1 = icmp slt i32 %rem943.i.1, 0
  %rem945.i.1 = select i1 %rem945.cmp.i.1, i32 0, i32 0
  %mul946.i.1 = mul i32 %rem945.i.1, 0
  %rem947.i.1 = urem i32 %mul946.i.1, 46337
  %rem956.cmp.i.1 = icmp ult i32 %rem947.i.1, 0
  %rem956.v.i.1 = select i1 %rem956.cmp.i.1, i32 0, i32 0
  %.neg = sub i32 0, 0
  %rem956.cmp.i = icmp ult i32 %rem947.i, 0
  %rem956.v.i = select i1 %rem956.cmp.i, i32 0, i32 0
  %0 = add i32 %.neg, %rem956.v.i
  %rem943.i.2 = srem i32 0, 46337
  %rem945.cmp.i.2 = icmp slt i32 %rem943.i.2, 0
  %rem945.i.2 = select i1 %rem945.cmp.i.2, i32 0, i32 0
  %mul946.i.2 = mul i32 %rem945.i.2, 0
  %rem947.i.2 = urem i32 %mul946.i.2, 46337
  %rem956.cmp.i.2 = icmp ult i32 %rem947.i.2, 0
  %rem956.v.i.2 = select i1 %rem956.cmp.i.2, i32 0, i32 0
  %1 = add i32 %rem956.v.i.1, %0
  %2 = add i32 %rem943.i.1, %1
  %3 = add i32 %rem956.v.i.2, %2
  %rem943.i.3 = srem i32 0, 46337
  %rem945.cmp.i.3 = icmp slt i32 %rem943.i.3, 0
  %rem945.i.3 = select i1 %rem945.cmp.i.3, i32 0, i32 0
  %mul946.i.3 = mul i32 %rem945.i.3, 0
  %rem947.i.3 = urem i32 %mul946.i.3, 46337
  %rem956.cmp.i.3 = icmp ult i32 %rem947.i.3, 0
  %rem956.v.i.3 = select i1 %rem956.cmp.i.3, i32 0, i32 0
  %.neg38 = sub i32 0, 0
  %4 = add i32 %3, 0
  %5 = add i32 %.neg38, %4
  %6 = add i32 %rem956.v.i.3, %5
  ret i32 %6
}
