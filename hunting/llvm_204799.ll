target triple = "x86_64-unknown-linux-gnu"

define i32 @main() {
entry:
  ret i32 0
}

define fastcc i32 @func_05wogj_0.specialized.10(i64 %indvars.iv3166) {
for_body_861.preheader3486:
  %reduc_2_64 = alloca [2 x i32], align 4
  br label %for_body_861

for_body_861:                                     ; preds = %for_body_861, %for_body_861.preheader3486
  br label %for_body_861

for_cond_873.loopexit:                            ; preds = %for_cond_873.loopexit
  store i32 %0, ptr %reduc_2_64, align 4
  %0 = load i32, ptr %reduc_2_64, align 4
  br label %for_cond_873.loopexit

vector.body3410:                                  ; preds = %for_body_895.us, %vector.body3410
  br label %vector.body3410

for_body_895.us:                                  ; preds = %for_body_895.us
  %arrayidx1473.us = getelementptr [4 x i8], ptr %reduc_2_64, i64 %indvars.iv3166
  %1 = load i32, ptr %arrayidx1473.us, align 4
  br i1 false, label %for_body_895.us, label %vector.body3410
}
