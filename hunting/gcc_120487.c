int a, b, c, d, e;
static void f(int g, int h) {
  int i;
  b = -999 - 999 * g - 999 - 2147483647;
  d = -999 * b - 2147483647;
j:
  i = e - 999 * g - 2147483647;
  c = -999 * g - 999 * e - 2147483647;
  if (c <= -300)
    return;
m:
  g = h - 999 * g;
  i = -d - 999 * i - 2147483647;
  if (-999 * i - g - c - 1080117449 >= 0)
    goto j;
  while (1) {
    a = i;
    if (g + 36057402 * i)
      goto m;
  }
}
int main() {
  f(-2149633, -2147483647);
  if (a != 0)
    __builtin_abort();
  return 0;
}