int a[256];
int d = -1, e, f, k, l, m, c, g = -1, h, n, p, q, r;
int o[27];

int t(int u, char v) { return (u >> 8 & 16777215) ^ a[(u ^ v) & 255]; }

int w(int u, int v[]) {
  for (; e < u; ++e) {
    f = (d >> 8 & 16777215) ^ a[(d ^ v[e]) & 255];
    k = f;
    k = t(k, v[e] >> 8);
    k = t(k, v[e] >> 16);
    k = t(k, v[e] >> 24);
    d = k;
  }
  d = d ^ 4294967295U;
  return d;
}

int x(int u, int v[], int y[][3]) {
  u = (1559566456 & u) - 561617765;
  v[0] = ~l;
  m = -(1383710289 - v[0]);
  int b[] = {l, m, 291686388, u, 0, y[0][0], y[0][1], y[0][2]};
  return w(sizeof(b) / sizeof(int), b);
}

void z(int u[], int v, int y[]) {
  c = (u[0] >> (x(1084390451, (int[1]){0}, (int[1][3]){{n, 0, 741428939}}) - 697008554)) -
      (-536920058 + v - 1);
  r = (21451 * (c % 46337) % 46337 * (y[1] + 46337) + 38939 * g) % 46337;
  q = -13363 + r;
  p = -71005931 * q + ~51083117 + 1968243255;
aa:
  if (p - 12)
    goto ab;
  goto ac;
ab:
  o[p] = 1 + p;
  p = 1 + p;
  goto aa;
ac:
  h = o[1];
}

int main() {
  for (int i = 0; i < 256; i++) {
    unsigned s = i;
    for (int j = 8; j; j--)
      if (s & 1)
        s = s >> 1 ^ 3988292384U;
      else
        s >>= 1;
    a[i] = s;
  }
  z((int[]){196095}, 49151, (int[]){1, -1});
}
