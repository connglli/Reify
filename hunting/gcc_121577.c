int a[] = {-3, 2, -2}, b, d, e;

void f(int g, int h) {
  int i = 12 / h;
  do {
    if (5 / i)
      g = 0;
    e = 5 / i - 6;
  } while (2 / e);
  b = 2 - 5 % i;
  int j[] = {b, 2, g};
  for (int c = 0; c < 3; c++)
    if (j[c] != a[c])
      __builtin_abort();
}

int main() {
  d = 2;
  f(-2, d);
  return 0;
}
