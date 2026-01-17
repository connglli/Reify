int a, c, d, e;

int b(volatile int f, int g) {
  f = -2;
  if (!f)
    a = 1;
  c = 11 / f - g + 2;
  g = c + 2;
  if (!(1 / c + (g - 1)))
    a = 1;
  return a + f;
}

int main() {
  do {
    e = (b(1, -1) - 30885397) % 2 - 3;
    d = b(1, -1) - 1430885400;
  } while (a + 1 == 0);
  if (d + e != -1430885406)
    __builtin_abort();
  return 0;
}
