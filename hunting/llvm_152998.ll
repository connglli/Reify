; RUN: opt < %s -passes=licm -S | FileCheck %s
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main(ptr %h) {
entry:
  br label %while.cond

while.cond:                                       ; preds = %ac.exit, %entry
  br label %for.body.i

for.cond1.preheader.i:                            ; preds = %for.body.i
  br i1 false, label %ethread-pre-split.i, label %ethread-pre-split.thread.i

for.body.i:                                       ; preds = %for.body.i, %while.cond
  %dec.i1 = phi i32 [ 1, %for.body.i ], [ 0, %while.cond ]
  store i32 %dec.i1, ptr %h, align 4
  br i1 false, label %for.cond1.preheader.i, label %for.body.i

ethread-pre-split.i:                              ; preds = %ethread-pre-split.split.i, %for.cond1.preheader.i
  br i1 false, label %ac.exit, label %ethread-pre-split.split.i

ethread-pre-split.thread.i:                       ; preds = %for.cond1.preheader.i
  br i1 false, label %ac.exit, label %ethread-pre-split.split.i

ethread-pre-split.split.i:                        ; preds = %ethread-pre-split.thread.i, %ethread-pre-split.i
  br i1 false, label %ethread-pre-split.split.split.i, label %ethread-pre-split.i

ethread-pre-split.split.split.i:                  ; preds = %ethread-pre-split.split.i
  store i32 0, ptr null, align 4
  ret i32 0

ac.exit:                                          ; preds = %ethread-pre-split.thread.i, %ethread-pre-split.i
  store i32 0, ptr null, align 4
  br label %while.cond
}
