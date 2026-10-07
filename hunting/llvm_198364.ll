target triple = "x86_64-unknown-linux-gnu"

define i1 @main() {
entry:
  %rem1857 = srem i32 0, 46337
  %rem1859 = select i1 false, i32 0, i32 %rem1857
  %.fr3 = freeze i32 0
  %rem1863 = srem i32 %.fr3, 46337
  %rem1865 = select i1 false, i32 0, i32 %rem1863
  %rem1917 = srem i32 0, 46337
  %rem1919 = select i1 false, i32 0, i32 %rem1917
  %rem1876.urem = xor i32 poison, 0
  %rem1876 = select i1 false, i32 0, i32 %rem1876.urem
  %mul2037 = mul i32 %rem1876, 0
  br label %if.then2378

if.then2378:                                      ; preds = %entry
  %rem1945 = urem i32 0, 46337
  %mul1950 = mul i32 %rem1859, %rem1945
  %rem1951 = urem i32 %mul1950, 46337
  %add1964 = or i32 %rem1951, 0
  %add1964.frozen = freeze i32 %add1964
  %rem1965 = select i1 false, i32 %add1964.frozen, i32 0
  %rem1974 = urem i32 %rem1965, 46337
  %rem1976.cmp = icmp ult i32 %rem1974, 0
  %rem2191 = urem i32 0, 46337
  %mul2197 = mul i32 %rem2191, %rem1919
  %rem2198 = urem i32 %mul2197, 46337
  %add2211.frozen = freeze i32 %rem2198
  %rem2212 = select i1 false, i32 %add2211.frozen, i32 0
  %rem2220 = urem i32 %rem2212, 46337
  %rem2222.cmp = icmp ult i32 %rem2220, 0
  %rem1861 = urem i32 0, 46337
  %mul1866 = mul i32 %rem1861, %rem1865
  %mul1877 = mul i32 %rem1876, 0
  %rem1878 = urem i32 %mul1877, 46337
  %rem1867 = urem i32 %mul1866, 46337
  %add1879 = add i32 %rem1867, %rem1878
  %add1879.frozen = freeze i32 %add1879
  %rem1880 = select i1 false, i32 %add1879.frozen, i32 0
  %rem1888 = urem i32 %rem1880, 46337
  %rem1890.cmp = icmp ult i32 %rem1888, 0
  %rem1902 = urem i32 0, 46337
  %mul1907 = mul i32 %rem1902, %rem1865
  %rem1908 = urem i32 %mul1907, 46337
  %mul1920 = mul i32 %rem1919, 0
  %rem1921 = urem i32 %mul1920, 46337
  %add1922 = add i32 %rem1908, %rem1921
  %add1922.frozen = freeze i32 %add1922
  %rem1923 = select i1 false, i32 %add1922.frozen, i32 0
  %rem1931 = urem i32 %rem1923, 46337
  %rem1933.cmp = icmp ult i32 %rem1931, 0
  ret i1 %rem1933.cmp
}
