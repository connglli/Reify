int a, b, c, ae, af;
int d(int e, int f, int ag) {
g:
  if (f <= 0)
    goto j;
  c = -2 - e - 227068031;
  if (a + 7)
    goto h;
  goto j;
h:
  ae = -e - 1920415615 * c + 2147483648;
i:
  if (b - ag - f + 1290222195 >= 0)
    goto j;
  if (ae)
    return ae;
  goto g;
ai:
  af = 558007421 * ag;
  if (af >= 0)
    goto i;
  e = 0;
  goto h;
j:
  ag = ag - 184330119 * ae;
  goto ai;
}
__attribute__((noinline))
int f() {
  int ak = d(-227068032, 1290222211, -184330117);
  if (ak != -1)
    __builtin_abort();
  return 0;
}
int main()
{
  return f();
}