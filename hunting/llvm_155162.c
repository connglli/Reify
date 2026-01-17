int a, b;

void c(int *d, int e) {
  int f = 0;
  for (b = 0; b < e; b++)
    for (f = 0; f < 64; f++)
      if (d[b] << f)
        return;
}

int g(int h) {
  int i[] = {h};
  c(i, 8);
  return a;
}

int main() {
  if (g(1) != 0)
    __builtin_abort();
  return 0;
}
