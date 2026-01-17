target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @ak() {
entry:
  %call34 = call i32 @ab(i32 1)
  %call223 = call i32 @ab(i32 0)
  %tobool3.not = icmp eq i32 %call223, 0
  br i1 %tobool3.not, label %common.ret, label %if.then4

common.ret:                                       ; preds = %entry
  ret i32 0

if.then4:                                         ; preds = %entry
  %call512 = call i32 @ab(i32 0)
  ret i32 %call512
}

define internal i32 @ab(i32 %ac) {
entry:
  br label %ai

ai:                                               ; preds = %ai, %entry
  %add37 = or i32 0, 0
  %tobool52.not = icmp eq i32 %ac, 1
  br i1 %tobool52.not, label %aj, label %ai

aj:                                               ; preds = %ai
  ret i32 0
}
