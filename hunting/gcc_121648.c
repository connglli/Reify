int a = -2;

int main() {
  int b = -2, c = 12 / a;
  do {
    if (3 / c)
      b = 0;
  } while (3 / c);
  int d[] = {b, -2, 1 - 3 % c};
  for (int e = 0; e < 3; e++)
    if (d[e] != a)
      __builtin_abort();
  return 0;
}
