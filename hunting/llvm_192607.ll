define i32 @foo(i32 %0) {
  %2 = add i32 %0, 1
  %4 = insertelement <2 x i32> zeroinitializer, i32 %2, i64 0
  %5 = add nuw <2 x i32> %4, zeroinitializer
  ret i32 %2
}

define i32 @main() {
  %1 = call i32 @foo(i32 -1)
  ret i32 %1
}
