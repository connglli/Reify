int a, b, e, f, h, i;

void d(int n[]) {
  for (int c = 0; c < 10; c++)
    if (n[c])
      a = 0;
}

void g(int n[]) {
  for (int c = 0; c < 4; c++)
    if (n[c])
      return;
  __builtin_abort();
}

int main() {
  int k[2] = {0, 0};
  k[f] = 0;
  while (h)
    d((int[]){k[1]});
  if (a) {
    int j = i;
    if (i < 0)
      goto l;
    goto m;
  l:
    j = 0;
    if (e) {
    m:
      if (e)
        goto l;
    }
    g((int[]){j});
  }
}
