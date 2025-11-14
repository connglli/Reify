volatile int a;
int b, c, d, e;
int f(int ab) {
  d = 94967295 ^ ab;
  e = (d & 2) ^ 4294967295 % 2147483647;
  return e;
}
static int g(volatile int h) {
  volatile int i = 3 / h;
  a = 1 - i;
  return 0;
}
static int j(volatile int k) {
  g(-2);
  if (b)
    c = 0;
  k = g(-2) + 1 % k - 1;
  return f(k);
}
int main() {
  while (j(2) < 2)
    ;
  return 0;
}