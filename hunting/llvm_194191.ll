target triple = "x86_64"

%struct.S2 = type { i32, i32, [9 x %struct.S1] }
%struct.S1 = type { i32, i32, i32 }

@global_struct = internal global %struct.S2 zeroinitializer, align 8


define i32 @main() {
  %2 = call i32 @foo(i32 0, ptr @global_struct, i1 false)
  ret i32 %2
}

define i32 @foo(i32 %v2, ptr byval(%struct.S2) align 8 %v5, i1 %cmp3644) {
entry:
  br label %BB4

BB4:                                              ; preds = %BB15, %entry
  br label %BB5

BB5:                                              ; preds = %BB6.preheader, %BB4
  %rem2832 = srem i32 %v2, 46337
  %f2297 = getelementptr i8, ptr %v5, i64 112
  %0 = load i32, ptr %f2297, align 8
  %.fr5712 = freeze i32 %0
  %rem2869 = srem i32 %.fr5712, 46337
  %add2870 = or i32 %rem2869, 0
  %rem2871.cmp = icmp slt i32 %rem2869, 0
  %rem2871 = select i1 %rem2871.cmp, i32 %add2870, i32 %rem2869
  %mul2872 = mul i32 0, %rem2871
  %rem2873 = urem i32 %mul2872, 46337
  %rem2834.cmp = icmp slt i32 %rem2832, 0
  %rem2834 = select i1 %rem2834.cmp, i32 %rem2832, i32 0
  %mul2877 = mul i32 %rem2873, %rem2834
  %rem2878 = urem i32 %mul2877, 46337
  %add2897.frozen = freeze i32 %rem2878
  %rem2898 = select i1 false, i32 %add2897.frozen, i32 0
  %rem2910 = urem i32 %rem2898, 46337
  %rem2912.cmp = icmp ult i32 %rem2910, 0
  %f1783 = getelementptr i8, ptr %v5, i64 108
  %f2354 = getelementptr i8, ptr %v5, i64 88
  %1 = load i32, ptr %f2354, align 8
  %.fr5727 = freeze i32 %1
  %rem3156 = srem i32 %.fr5727, 46337
  %rem3158.cmp = icmp slt i32 %rem3156, 0
  %rem3158 = select i1 %rem3158.cmp, i32 %rem3156, i32 %rem3156
  %mul3159 = mul i32 %rem3158, 0
  %rem3160 = urem i32 %mul3159, 46337
  %mul3168 = mul i32 %rem3160, %rem3158
  %rem3169 = urem i32 %mul3168, 46337
  %add3183.frozen = freeze i32 %rem3169
  %rem3184 = select i1 false, i32 %add3183.frozen, i32 0
  %rem3196 = urem i32 %rem3184, 46337
  %rem3198.cmp = icmp ult i32 %rem3196, 0
  %arrayidx6 = getelementptr i8, ptr %v5, i64 80
  %2 = load i32, ptr %arrayidx6, align 8
  %.fr5729 = freeze i32 %2
  %rem3205 = srem i32 %.fr5729, 1
  %rem3207 = select i1 false, i32 0, i32 %rem3205
  %mul3219 = mul i32 0, %rem3207
  %rem3220 = urem i32 %mul3219, 46337
  %mul3228 = mul i32 %rem3220, %rem2871
  %rem3229 = urem i32 %mul3228, 46337
  %add3246.frozen = freeze i32 %rem3229
  %rem3247 = select i1 false, i32 %add3246.frozen, i32 0
  %rem3259 = urem i32 %rem3247, 46337
  %rem3261.cmp = icmp ult i32 %rem3259, 0
  %f1213 = getelementptr i8, ptr %v5, i64 4
  %3 = load i32, ptr %f1213, align 4
  %.fr5719 = freeze i32 %3
  %rem2962 = srem i32 %.fr5719, 1
  %rem2964 = select i1 false, i32 0, i32 %rem2962
  %rem3301 = srem i32 %v2, 46337
  %rem3303.cmp = icmp slt i32 %rem3301, 0
  %add3302 = or i32 %rem3301, 0
  %rem3303 = select i1 %rem3303.cmp, i32 %add3302, i32 0
  %mul3293 = mul i32 %rem2964, 0
  %rem3294 = urem i32 %mul3293, 46337
  %mul3304 = mul i32 %rem3303, %rem3294
  %rem3305 = urem i32 %mul3304, 46337
  %add3306.frozen = freeze i32 %rem3305
  %rem3307 = select i1 false, i32 %add3306.frozen, i32 0
  %rem3314 = urem i32 %rem3307, 46337
  %rem3316.cmp = icmp ult i32 %rem3314, 0
  %4 = load i32, ptr %v5, align 4
  %.fr5733 = freeze i32 %4
  %rem3353 = srem i32 %.fr5733, 1
  %rem3355 = select i1 false, i32 0, i32 %rem3353
  %mul3356 = mul i32 %rem3355, 0
  %rem3357 = urem i32 %mul3356, 46337
  %mul3361 = mul i32 %rem3357, %rem2834
  %rem3362 = urem i32 %mul3361, 46337
  %add3363.frozen = freeze i32 %rem3362
  %rem3364 = select i1 false, i32 %add3363.frozen, i32 0
  %rem3377 = urem i32 %rem3364, 46337
  %rem3379.cmp = icmp ult i32 %rem3377, 0
  %arrayidx62 = getelementptr i8, ptr %v5, i64 56
  %5 = load i32, ptr %arrayidx62, align 8
  %.fr5734 = freeze i32 %5
  %rem3409 = srem i32 %.fr5734, 1
  %rem3411 = select i1 false, i32 0, i32 %rem3409
  %mul3412 = mul i32 %rem3411, 0
  %rem3413 = urem i32 %mul3412, 46337
  %mul3421 = mul i32 %rem3413, %rem3158
  %rem3422 = urem i32 %mul3421, 46337
  %add3423.frozen = freeze i32 %rem3422
  %rem3424 = select i1 false, i32 %add3423.frozen, i32 0
  %rem3435 = urem i32 %rem3424, 46337
  %rem3437.cmp = icmp ult i32 %rem3435, 0
  %f2882 = getelementptr i8, ptr %v5, i64 100
  %6 = load i32, ptr %f2882, align 4
  %.fr5736 = freeze i32 %6
  %rem3450 = srem i32 %.fr5736, 46337
  %rem3452.cmp = icmp slt i32 %rem3450, 0
  %add3451 = or i32 %rem3450, 0
  %rem3452 = select i1 %rem3452.cmp, i32 %add3451, i32 %rem3450
  %mul3453 = mul i32 %rem3452, 0
  %rem3454 = urem i32 %mul3453, 46337
  %mul3462 = mul i32 %rem3454, %rem3452
  %rem3463 = urem i32 %mul3462, 46337
  %add3480.frozen = freeze i32 %rem3463
  %rem3481 = select i1 false, i32 %add3480.frozen, i32 0
  %rem3493 = urem i32 %rem3481, 46337
  %rem3495.cmp = icmp ult i32 %rem3493, 0
  %7 = load i32, ptr %f1783, align 4
  %.fr5726 = freeze i32 %7
  %rem3110 = srem i32 %.fr5726, 46337
  %rem3112.cmp = icmp slt i32 %rem3110, 0
  %rem3112 = select i1 %rem3112.cmp, i32 %rem3110, i32 %rem3110
  %mul3515 = mul i32 0, %rem3112
  %rem3516 = urem i32 %mul3515, 46337
  %mul3525 = mul i32 %rem3516, %rem3112
  %rem3526 = urem i32 %mul3525, 46337
  %add3545.frozen = freeze i32 %rem3526
  %rem3546 = select i1 false, i32 %add3545.frozen, i32 0
  %rem3553 = urem i32 %rem3546, 46337
  %rem3555.cmp = icmp ult i32 %rem3553, 0
  br i1 %cmp3644, label %BB6.preheader, label %BB18

BB6.preheader:                                    ; preds = %BB5
  br i1 false, label %BB5, label %BB15

BB12:                                             ; preds = %BB15
  %sub3943.neg = sub i32 0, 0
  %mul3941.neg = mul i32 0, 1
  %sub3944 = or i32 %sub3943.neg, %mul3941.neg
  %cmp3946 = icmp eq i32 %sub3944, 0
  br label %BB15

BB15:                                             ; preds = %BB12, %BB6.preheader
  br i1 false, label %BB4, label %BB12

BB18:                                             ; preds = %BB5
  ret i32 %mul3168

; uselistorder directives
  uselistorder ptr %v5, { 0, 1, 2, 6, 3, 4, 5, 7 }
}
