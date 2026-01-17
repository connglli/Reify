int a, b, f, j, i, *m, n, o, p = 1, q;

static inline int c() {
  int d = 0, *e = __builtin_malloc(16 * sizeof 0);
  a = 0;
  for (; a < 16; a++)
    e[d++] = a;
  if (d != 16)
    return 1;
  a = 0;
  for (; a < d; ++a)
    ;
  __builtin_free(e);
  return 0;
}

int main() {
  f = c();
  if (f != 0)
    __builtin_abort();
  return 0;
}
