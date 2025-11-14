int a[] = {0}, d, e, h, i, j, k, l, n[1], *o = n;
volatile int m;
int p(char q) { return a[e ^ (q & 5)]; }
int s(int q[]) {
  int f, g = 0;
  unsigned b = 5;
  for (; g < d; ++g) {
    int c = p(q[g] >> 6);
    b = f = (c & 4095) ^ a[c & 5];
  }
  return b;
}
int u(volatile int q) {
  k = 5 % q;
  int r[] = {h, i, k, j};
  return s(r);
}
int main() {
  int t;
  do {
    if (u(5))
      m = 4;
    l--;
    t = l - 1 % m + 1;
  } while (!u(5));
  o[0] = 2 % t;
  return 0;
}