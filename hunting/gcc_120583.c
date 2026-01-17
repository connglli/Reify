int a, b, c;

static int d(int e) {
  do {
    c = a = -998 * b - 5000000;
    b = e + b - 2148000;
  } while (c <= 0 && 10 * b - a <= 0);
  return c;
}

int main() {
  if (a)
    d(1);
  if (d(0) != 2138704000)
    __builtin_abort();
  return 0;
}
