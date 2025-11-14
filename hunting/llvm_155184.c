int a, b, d, g, ab, ae, af, h, i, j, l, n = 1;
int aa(int u[]) {
  for (int c = 0; c < 3; c++) {
    for (int e = 0; e < 11; e++)
      if (u[c + e]) {
        n = 0;
        break;
      }
    if (n)
      return a;
  }
  __builtin_abort();
}
void f(int u, int x, int y) {
  int v, o = 0, p, q;
  p = u;
  if (j)
    goto r;
  do {
    v = u = x = 0;
    if (o)
      break;
  r:
    o = x;
  } while (j);
  q = 0;
  int t[] = {u, h, x, i, q, v, j, l, y, o, p};
  aa(t);
}
int main() {
  ab = 1;
  for (; ab; ab = 0)
    for (; ae; ae++)
      while (1) {
        int m = 0;
        goto w;
      ac:
        m = 3;
      ad:
        if (1 % m)
          goto ac;
      w:
        if (g)
          goto ad;
        if (d)
          goto ac;
        int k = b;
        while (1)
          if (k)
            break;
        f(af, 1, af - 1);
        break;
      }
  return 0;
}