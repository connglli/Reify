target triple = "x86_64-unknown-linux-gnu"

define i32 @func_kigqpi_31() {
entry:
  %rem2597.cmp = icmp ugt i32 1, 0
  %rem2597.urem.neg = zext i1 %rem2597.cmp to i32
  %reass.sub4418 = sub i32 %rem2597.urem.neg, 1
  %shl2601 = shl i32 %reass.sub4418, 31
  br label %for_cond_8980.loopexit

for_cond_8980.loopexit:                           ; preds = %for_cond_8980.loopexit, %entry
  %arg_11.04280 = phi i32 [ %add2627, %for_cond_8980.loopexit ], [ 0, %entry ]
  %i_1826.04279 = phi i32 [ %sub2691, %for_cond_8980.loopexit ], [ %shl2601, %entry ]
  %add2627 = or i32 %arg_11.04280, 0
  %sub2691 = add i32 %i_1826.04279, 1
  %cmp2624 = icmp slt i32 %i_1826.04279, 0
  br i1 %cmp2624, label %for_cond_8980.loopexit, label %for_exit_8980.loopexit

for_exit_8980.loopexit:                           ; preds = %for_cond_8980.loopexit
  ret i32 %add2627
}

define i32 @main() {
entry:
  %call475.i = call i32 @func_kigqpi_31()
  ret i32 %call475.i
}
