unsigned b[256], *c = b;
int d, e, f, g, h, l, ab, m, ae, n, af, ag, ah, ai, aj, o = -1, p, q = -1;

unsigned aa(unsigned t, char u) { return t >> 8 ^ b[(t ^ u) & 255]; }

unsigned k(unsigned t, unsigned u) {
  p = t >> 8 ^ b[(t ^ u) & 255];
  t = p;
  t = aa(t, u >> 8);
  t = aa(t, u >> 16);
  t = aa(t, u >> 4);
  return t;
}

int ac(int t) {
  d = -t - 1;
  f = (-1 - 1) * d - 1;
  int a[] = {g, d, e, t, f};
  unsigned ad = -1;
  for (int i = 0; i < 5; ++i)
    ad = k(ad, a[i]);
  ad = (ad ^ -1) % 2147483647;
  return ad;
}

int r(int t, int u, int v, int w, int x, int y, int z, int ak) {
  l = (ac(-2) - 58245758) * u;
  m = 1 - l - 1;
  ab = (ac(-1) - 1270062073) * w - 1;
  w = -z - w - 1;
  int a[] = {t, u, v, w, h, x, y, m, z, l, ab, ak};
  unsigned ad = -1;
  for (int i = 0; i < 12; ++i)
    ad = k(ad, a[i]);
  ad = (ad ^ -1) % 2147483647;
  return ad;
}

int am(int t, int u) {
  int s = -999 * t - 2147483647;
  if (t)
  al:
    t = r(-1, -1, -1, -1, -1, -1, -1, -1) - 92697922 - 999 * t - 2147483647;
  if ((ac(-1) - 1270062073) * t - 999 * u - 6205925 >= 0)
    goto an;
ao:
  if (n)
    s = 2147483647;
  goto al;
ap:
  t = -999 * u + t + 2144058291;
  if (-999 * t + 4 >= 0) {
    s = -999 * s - 998 * t - 2147483647;
    n = -999 - 999 * s - 1868323085;
    goto ao;
  }
an:
  ae = -999 * n - t - 2147483647;
  if ((r(-1, 1, -1, 1, 0, 1, 1, 1) - 131488391) * ae - 6443399 >= 0)
    goto ap;
  for (int i = 0; i < 5; ++i)
    o = k(o, s);
  o = o ^ 4294967295;
  return o;
}

int aq(int t, int u) {
  af = -1;
  int a[] = {f, t, af, u, ag};
  for (int i = 0; i < 5; ++i)
    q = k(q, a[i]);
  q = q ^ 4294967295;
  return q;
}

int main() {
  for (unsigned i = 0; i < 256; i++) {
    unsigned ar = i;
    for (int j = 8; j; j--)
      if (ar & 1)
        ar = ar >> 1 ^ 3988292384;
      else
        ar >>= 1;
    c[i] = ar;
  }
  ai = aq(-1, 1) - 1410405933;
  if ((am(-2149633, 1) - 836646560) * ai >= 0)
    aj = -2002;
  ah = (r(-1, -1, -1, -1, -1, -1, -1, -1) - 92696925) * aj;
  if ((ac(-1) - 1270062074) * ah >= 0)
    __builtin_abort();
}
