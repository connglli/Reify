@a = global i32 0, align 4
@b = global i32 0, align 4
@c = global i32 0, align 4
@d = global i32 0, align 4

define i32 @main() {
entry:
  br label %f

f:                                                ; preds = %o, %entry
  br i1 false, label %q, label %k

k:                                                ; preds = %s, %f
  store i32 1, ptr @c, align 4
  %cmp = icmp sgt i32 0, -1
  br i1 %cmp, label %o, label %q

s:                                                ; preds = %o, %if.then
  %2 = load i32, ptr @a, align 4
  %3 = load i32, ptr @c, align 4
  %mul7 = mul nsw i32 %3, %2
  store i32 %mul7, ptr @b, align 4
  %cmp8 = icmp sgt i32 %mul7, -1
  br i1 %cmp8, label %if.then, label %k

if.then:                                          ; preds = %s
  %mul = mul nsw i32 %3, 3
  store i32 %mul, ptr @c, align 4
  store i32 %3, ptr @d, align 4
  %mul3 = mul nsw i32 %3, %2
  %cmp4 = icmp sgt i32 %mul3, -1
  br i1 %cmp4, label %s, label %q

o:                                                ; preds = %k
  %4 = load i32, ptr @d, align 4
  store i32 %4, ptr @b, align 4
  %tobool12.not = icmp eq i32 %4, 0
  br i1 %tobool12.not, label %s, label %f

q:                                                ; preds = %f, %if.then, %k
  ret i32 0
}