; bin/opt -passes=slp-vectorizer reduced.ll -S
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main(i32 %mul) {
entry:
  %h = alloca [4 x i32], align 16
  %add = add i32 0, 0
  %add4 = add i32 %add, 0
  %call = tail call i32 @f1(i32 %add4)
  %mul1 = shl i32 0, 1
  %add5 = add i32 %call, %mul1
  store i32 %add5, ptr %h, align 16
  %arrayinit.element = getelementptr i8, ptr %h, i64 4
  %add6 = add i32 0, 0
  %add7 = add i32 %add6, %mul
  %add9 = add i32 %add7, %add4
  store i32 %add9, ptr %arrayinit.element, align 4
  %arrayinit.element10 = getelementptr i8, ptr %h, i64 8
  %add11 = or i32 %add, 0
  %add12 = add i32 %add11, %add4
  store i32 %add12, ptr %arrayinit.element10, align 8
  %arrayinit.element13 = getelementptr i8, ptr %h, i64 12
  store i32 0, ptr %arrayinit.element13, align 4
  ret i32 0
}

declare i32 @f1(i32)
