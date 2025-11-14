int a = 1, b, c;
int main() {
  b = -5001001 * a + 5001000;
  while (b >= 5001001)
    b = a + 5001000;
  c = -5001000 * b - 5001001;
  if (5001000 * c >= b)
    __builtin_abort();
  return 0;
}