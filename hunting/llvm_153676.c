extern int fn1(int);
extern void fn2(int *);
extern int fn3(void);
int a, b;

void d(int e) {
  int f = a, g = 1 + (fn3() + a + b);
  int h[] = {b + g + a, e + f + g, f + g + e, fn1(g + 1)};
  fn2(h);
}
